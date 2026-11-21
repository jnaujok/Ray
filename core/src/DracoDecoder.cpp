/*************************************************************************
*   DracoDecoder                                                         *
*                                                                        *
*   See DracoDecoder.h. The names follow Draco's own (corner tables,     *
*   traversers, prediction schemes) so the code can be read beside the   *
*   bitstream specification. Where the 2017 specification and Google's   *
*   decoder disagree, this follows the decoder, which wrote the files:   *
*   bits are read least significant first; values are decoded even with *
*   no prediction; the octahedron transforms read no center value in     *
*   2.2; the valence traversal has a mode byte; a vertex is on a         *
*   boundary when its left-most corner can't swing further left.         *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#include "DracoDecoder.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <memory>

namespace
{
    const int INVALID = -1;

    enum { TOPOLOGY_C = 0, TOPOLOGY_S = 1, TOPOLOGY_L = 3, TOPOLOGY_R = 5, TOPOLOGY_E = 7 };
    enum { MESH_SEQUENTIAL = 0, MESH_EDGEBREAKER = 1 };
    enum { EDGEBREAKER_STANDARD = 0, EDGEBREAKER_VALENCE = 2 };
    enum { DECODER_GENERIC = 0, DECODER_INTEGER = 1, DECODER_QUANTIZATION = 2, DECODER_NORMALS = 3 };
    enum { PREDICTION_NONE = -2, PREDICTION_DIFFERENCE = 0, PREDICTION_PARALLELOGRAM = 1,
           PREDICTION_CONSTRAINED_MULTI = 4, PREDICTION_TEX_COORDS_PORTABLE = 5, PREDICTION_GEOMETRIC_NORMAL = 6 };
    enum { TRANSFORM_WRAP = 1, TRANSFORM_OCTAHEDRON = 2, TRANSFORM_OCTAHEDRON_CANONICALIZED = 3 };
    enum { TRAVERSAL_DEPTH_FIRST = 0, TRAVERSAL_PREDICTION_DEGREE = 1 };
    enum { MESH_VERTEX_ATTRIBUTE = 0, MESH_CORNER_ATTRIBUTE = 1 };
    const int MAX_PARALLELOGRAMS = 4;

    struct SFail { int nLine = 0; };    // nLine: where it was thrown, for debugging

        // Reads the bitstream. Running past the end throws SFail.
    class CReader
    {
        public:
            CReader( const unsigned char *pData, size_t nSize ) : m_pData( pData ), m_nSize( nSize ), m_nAt( 0 ),
                m_nBitStart( 0 ), m_nBits( 0 ) {}

            size_t  At( void ) const { return m_nAt; }
            size_t  Left( void ) const { return m_nSize - m_nAt; }
            const unsigned char *Here( void ) const { return m_pData + m_nAt; }
            void    Skip( uint64_t nBytes ) { if ( nBytes > Left() ) throw SFail{ __LINE__ }; m_nAt += (size_t)nBytes; }

            uint8_t U8( void ) { if ( m_nAt >= m_nSize ) throw SFail{ __LINE__ }; return m_pData[ m_nAt++ ]; }
            int8_t  I8( void ) { return (int8_t)U8(); }
            uint32_t U32( void ) { uint32_t n = 0; for ( int i = 0; i < 4; i++ ) n |= (uint32_t)U8() << ( 8 * i ); return n; }
            int32_t I32( void ) { return (int32_t)U32(); }
            float   F32( void ) { uint32_t n = U32(); float f; memcpy( &f, &n, 4 ); return f; }
            uint64_t Var( void )
            {
                uint64_t nValue = 0;
                int      nShift = 0;

                while ( true )
                {
                    uint8_t nByte = U8();

                    if ( nShift >= 64 )
                        throw SFail{ __LINE__ };
                    nValue |= (uint64_t)( nByte & 0x7F ) << nShift;
                    if ( ( nByte & 0x80 ) == 0 )
                        return nValue;
                    nShift += 7;
                }
            }
            uint32_t Var32( void ) { uint64_t n = Var(); if ( n > 0xFFFFFFFFull ) throw SFail{ __LINE__ }; return (uint32_t)n; }

                // Bit decoding, least significant bit of each byte first.
            void    StartBits( void ) { m_nBitStart = m_nAt; m_nBits = 0; }
            uint32_t Bits( int nCount )
            {
                uint32_t nValue = 0;

                for ( int nBit = 0; nBit < nCount; nBit++ )
                {
                    size_t nByte = m_nBitStart + ( m_nBits >> 3 );
                    uint32_t nOne = ( nByte < m_nSize ) ? ( ( m_pData[ nByte ] >> ( m_nBits & 7 ) ) & 1u ) : 0u;

                    nValue |= nOne << nBit;
                    m_nBits++;
                }
                return nValue;
            }
            void    EndBits( void ) { Skip( ( m_nBits + 7 ) / 8 ); }

        private:
            const unsigned char *m_pData;
            size_t               m_nSize;
            size_t               m_nAt;
            size_t               m_nBitStart;
            uint64_t             m_nBits;
    };

        // rANS over a table of symbol probabilities.
    class CRansSymbols
    {
        public:
            uint32_t                m_nSymbols = 0;

            void Create( CReader &oReader, int nPrecisionBits )
            {
                m_nPrecision = 1u << nPrecisionBits;
                m_nBase = m_nPrecision * 4;
                m_nSymbols = oReader.Var32();
                if ( m_nSymbols > ( 1u << 20 ) )
                    throw SFail{ __LINE__ };
                m_anProb.assign( m_nSymbols, 0 );
                for ( uint32_t i = 0; i < m_nSymbols; i++ )
                {
                    uint8_t nData = oReader.U8();
                    int     nToken = nData & 3;

                    if ( nToken == 3 )
                    {
                        uint32_t nOffset = nData >> 2;

                        if ( i + nOffset >= m_nSymbols )
                            throw SFail{ __LINE__ };
                        i += nOffset;   // those are zero already
                    }
                    else
                    {
                        uint32_t nProb = nData >> 2;

                        for ( int j = 0; j < nToken; j++ )
                            nProb |= (uint32_t)oReader.U8() << ( 8 * ( j + 1 ) - 2 );
                        m_anProb[ i ] = nProb;
                    }
                }
                m_anCumulative.assign( m_nSymbols, 0 );
                m_anLookup.assign( m_nPrecision, 0 );

                uint32_t nCumulative = 0;

                for ( uint32_t i = 0; i < m_nSymbols; i++ )
                {
                    m_anCumulative[ i ] = nCumulative;
                    if ( (uint64_t)nCumulative + m_anProb[ i ] > m_nPrecision )
                        throw SFail{ __LINE__ };
                    for ( uint32_t j = nCumulative; j < nCumulative + m_anProb[ i ]; j++ )
                        m_anLookup[ j ] = i;
                    nCumulative += m_anProb[ i ];
                }
                if ( nCumulative != m_nPrecision )
                    throw SFail{ __LINE__ };
            }
            void Start( CReader &oReader )
            {
                uint64_t nBytes = oReader.Var();

                if ( nBytes > oReader.Left() || nBytes < 1 )
                    throw SFail{ __LINE__ };
                m_pBuffer = oReader.Here();
                oReader.Skip( nBytes );

                int      nOffset = (int)nBytes;
                uint32_t x = m_pBuffer[ nOffset - 1 ] >> 6;

                if ( x == 0 )
                {
                    m_nOffset = nOffset - 1;
                    m_nState = m_pBuffer[ nOffset - 1 ] & 0x3F;
                }
                else if ( x == 1 )
                {
                    if ( nOffset < 2 ) throw SFail{ __LINE__ };
                    m_nOffset = nOffset - 2;
                    m_nState = ( m_pBuffer[ nOffset - 2 ] | ( (uint32_t)m_pBuffer[ nOffset - 1 ] << 8 ) ) & 0x3FFF;
                }
                else if ( x == 2 )
                {
                    if ( nOffset < 3 ) throw SFail{ __LINE__ };
                    m_nOffset = nOffset - 3;
                    m_nState = ( m_pBuffer[ nOffset - 3 ] | ( (uint32_t)m_pBuffer[ nOffset - 2 ] << 8 ) |
                                 ( (uint32_t)m_pBuffer[ nOffset - 1 ] << 16 ) ) & 0x3FFFFF;
                }
                else
                {
                    if ( nOffset < 4 ) throw SFail{ __LINE__ };
                    m_nOffset = nOffset - 4;
                    m_nState = ( m_pBuffer[ nOffset - 4 ] | ( (uint32_t)m_pBuffer[ nOffset - 3 ] << 8 ) |
                                 ( (uint32_t)m_pBuffer[ nOffset - 2 ] << 16 ) | ( (uint32_t)m_pBuffer[ nOffset - 1 ] << 24 ) ) & 0x3FFFFFFF;
                }
                m_nState += m_nBase;
                if ( (uint64_t)m_nState >= (uint64_t)m_nBase * 256 )
                    throw SFail{ __LINE__ };
            }
            uint32_t Read( void )
            {
                while ( m_nState < m_nBase && m_nOffset > 0 )
                    m_nState = m_nState * 256 + m_pBuffer[ --m_nOffset ];

                uint32_t nQuotient = m_nState / m_nPrecision;
                uint32_t nRemainder = m_nState % m_nPrecision;
                uint32_t nSymbol = m_anLookup[ nRemainder ];

                m_nState = nQuotient * m_anProb[ nSymbol ] + nRemainder - m_anCumulative[ nSymbol ];
                return nSymbol;
            }

        private:
            uint32_t                m_nPrecision = 4096;
            uint32_t                m_nBase = 16384;
            std::vector<uint32_t>   m_anProb;
            std::vector<uint32_t>   m_anCumulative;
            std::vector<uint32_t>   m_anLookup;
            const unsigned char    *m_pBuffer = NULL;
            int                     m_nOffset = 0;
            uint32_t                m_nState = 0;
    };

        // Draco's binary rANS ("RAnsBitDecoder"): a zero probability byte,
        // a size and the data.
    class CRansBits
    {
        public:
            void Start( CReader &oReader )
            {
                m_nProbZero = oReader.U8();

                uint64_t nBytes = oReader.Var();

                if ( nBytes > oReader.Left() || nBytes < 1 )
                    throw SFail{ __LINE__ };
                m_pBuffer = oReader.Here();
                oReader.Skip( nBytes );

                int      nOffset = (int)nBytes;
                uint32_t x = m_pBuffer[ nOffset - 1 ] >> 6;

                if ( x == 0 )
                {
                    m_nOffset = nOffset - 1;
                    m_nState = m_pBuffer[ nOffset - 1 ] & 0x3F;
                }
                else if ( x == 1 )
                {
                    if ( nOffset < 2 ) throw SFail{ __LINE__ };
                    m_nOffset = nOffset - 2;
                    m_nState = ( m_pBuffer[ nOffset - 2 ] | ( (uint32_t)m_pBuffer[ nOffset - 1 ] << 8 ) ) & 0x3FFF;
                }
                else if ( x == 2 )
                {
                    if ( nOffset < 3 ) throw SFail{ __LINE__ };
                    m_nOffset = nOffset - 3;
                    m_nState = ( m_pBuffer[ nOffset - 3 ] | ( (uint32_t)m_pBuffer[ nOffset - 2 ] << 8 ) |
                                 ( (uint32_t)m_pBuffer[ nOffset - 1 ] << 16 ) ) & 0x3FFFFF;
                }
                else
                    throw SFail{ __LINE__ };
                m_nState += 4096;
                if ( m_nState >= 4096u * 256u )
                    throw SFail{ __LINE__ };
                m_bStarted = true;
            }
            bool Next( void )
            {
                if ( ! m_bStarted )
                    throw SFail{ __LINE__ };

                uint32_t p = 256 - m_nProbZero;

                if ( m_nState < 4096 && m_nOffset > 0 )
                    m_nState = m_nState * 256 + m_pBuffer[ --m_nOffset ];

                uint32_t x = m_nState;
                uint32_t nQuotient = x / 256;
                uint32_t nRemainder = x % 256;
                uint32_t xn = nQuotient * p;
                bool     bValue = nRemainder < p;

                if ( bValue )
                    m_nState = xn + nRemainder;
                else
                    m_nState = x - xn - p;
                return bValue;
            }

        private:
            uint8_t                 m_nProbZero = 0;
            const unsigned char    *m_pBuffer = NULL;
            int                     m_nOffset = 0;
            uint32_t                m_nState = 0;
            bool                    m_bStarted = false;
    };

    void DecodeSymbols( CReader &oReader, uint32_t nValues, int nComponents, std::vector<uint32_t> &anOut )
    {
        anOut.assign( nValues, 0 );
        if ( nValues == 0 )
            return;

        uint8_t nScheme = oReader.U8();

        if ( nScheme == 0 )
        {
                // Tagged: a rANS-coded bit length for each value, then the
                // values' bits.
            CRansSymbols oTags;

            oTags.Create( oReader, 12 );
            if ( oTags.m_nSymbols == 0 )
                throw SFail{ __LINE__ };
            oTags.Start( oReader );
            oReader.StartBits();
            for ( uint32_t i = 0; i < nValues; i += (uint32_t)nComponents )
            {
                uint32_t nLength = oTags.Read();

                if ( nLength > 32 )
                    throw SFail{ __LINE__ };
                for ( int j = 0; j < nComponents && i + (uint32_t)j < nValues; j++ )
                    anOut[ i + (uint32_t)j ] = oReader.Bits( (int)nLength );
            }
            oReader.EndBits();
        }
        else if ( nScheme == 1 )
        {
            uint8_t nMaxBits = oReader.U8();

            if ( nMaxBits < 1 || nMaxBits > 18 )
                throw SFail{ __LINE__ };

            int nPrecision = ( 3 * nMaxBits ) / 2;

            if ( nPrecision > 20 ) nPrecision = 20;
            if ( nPrecision < 12 ) nPrecision = 12;

            CRansSymbols oSymbols;

            oSymbols.Create( oReader, nPrecision );
            if ( oSymbols.m_nSymbols == 0 )
                throw SFail{ __LINE__ };
            oSymbols.Start( oReader );
            for ( uint32_t i = 0; i < nValues; i++ )
                anOut[ i ] = oSymbols.Read();
        }
        else
            throw SFail{ __LINE__ };
    }

    inline int Next( int c ) { return ( c < 0 ) ? c : ( ( c % 3 == 2 ) ? c - 2 : c + 1 ); }
    inline int Previous( int c ) { return ( c < 0 ) ? c : ( ( c % 3 == 0 ) ? c + 2 : c - 1 ); }

        // A corner table: the vertex at each corner, the corner opposite
        // each, and each vertex's left-most corner. An attribute's table
        // shares the position table's opposites, cut at its seams.
    struct SCornerTable
    {
        std::vector<int>            anVertex;           // by corner
        std::vector<int>            anOpposite;         // by corner (the position table's)
        std::vector<int>            anLeftMost;         // by vertex
        const std::vector<char>    *pSeam = NULL;       // by corner: its opposite edge is a seam (attribute tables)

        int     Opposite( int c ) const
        {
            if ( c < 0 || (size_t)c >= anOpposite.size() )
                return INVALID;
            if ( pSeam && (*pSeam)[ c ] )
                return INVALID;
            return anOpposite[ c ];
        }
        int     Vertex( int c ) const { return ( c < 0 ) ? INVALID : anVertex[ c ]; }
        int     Vertices( void ) const { return (int)anLeftMost.size(); }
        int     LeftMost( int v ) const { return ( v < 0 || v >= Vertices() ) ? INVALID : anLeftMost[ v ]; }
        int     SwingRight( int c ) const { return Previous( Opposite( Previous( c ) ) ); }
        int     SwingLeft( int c ) const { return Next( Opposite( Next( c ) ) ); }
        int     RightCorner( int c ) const { return ( c < 0 ) ? INVALID : Opposite( Next( c ) ); }
        int     LeftCorner( int c ) const { return ( c < 0 ) ? INVALID : Opposite( Previous( c ) ); }
        bool    OnBoundary( int v ) const
        {
            int c = LeftMost( v );

            if ( c < 0 )
                return true;
            return SwingLeft( c ) < 0;
        }
    };

        // One attribute decoder's traversal: the order its values come in.
    struct STraversal
    {
        const SCornerTable *pTable = NULL;
        std::vector<int>    anValueCorner;      // the corner each value was first seen at
        std::vector<int>    anVertexValue;      // the value at each of the table's vertices
        std::vector<char>   abFaceVisited;
        std::vector<char>   abVertexVisited;

        void Visit( int nVertex, int nCorner )
        {
            anVertexValue[ nVertex ] = (int)anValueCorner.size();
            anValueCorner.push_back( nCorner );
        }
        bool FaceVisited( int nFace ) const { return nFace < 0 || abFaceVisited[ nFace ]; }

        void DepthFirst( int nCorner, std::vector<int> &anStack )
        {
            const SCornerTable &t = *pTable;

            if ( FaceVisited( nCorner / 3 ) )
                return;
            anStack.clear();
            anStack.push_back( nCorner );

            int nNext = t.Vertex( Next( nCorner ) );
            int nPrev = t.Vertex( Previous( nCorner ) );

            if ( nNext < 0 || nPrev < 0 )
                throw SFail{ __LINE__ };
            if ( ! abVertexVisited[ nNext ] )
            {
                abVertexVisited[ nNext ] = 1;
                Visit( nNext, Next( nCorner ) );
            }
            if ( ! abVertexVisited[ nPrev ] )
            {
                abVertexVisited[ nPrev ] = 1;
                Visit( nPrev, Previous( nCorner ) );
            }
            while ( ! anStack.empty() )
            {
                int c = anStack.back();
                int nFace = c / 3;

                if ( c < 0 || FaceVisited( nFace ) )
                {
                    anStack.pop_back();
                    continue;
                }
                while ( true )
                {
                    abFaceVisited[ nFace ] = 1;

                    int v = t.Vertex( c );

                    if ( v < 0 )
                        throw SFail{ __LINE__ };
                    if ( ! abVertexVisited[ v ] )
                    {
                        bool bBoundary = t.OnBoundary( v );

                        abVertexVisited[ v ] = 1;
                        Visit( v, c );
                        if ( ! bBoundary )
                        {
                            c = t.RightCorner( c );
                            nFace = c / 3;
                            continue;
                        }
                    }

                    int nRight = t.RightCorner( c );
                    int nLeft = t.LeftCorner( c );
                    int nRightFace = ( nRight < 0 ) ? -1 : nRight / 3;
                    int nLeftFace = ( nLeft < 0 ) ? -1 : nLeft / 3;

                    if ( FaceVisited( nRightFace ) )
                    {
                        if ( FaceVisited( nLeftFace ) )
                        {
                            anStack.pop_back();
                            break;
                        }
                        c = nLeft;
                        nFace = nLeftFace;
                    }
                    else if ( FaceVisited( nLeftFace ) )
                    {
                        c = nRight;
                        nFace = nRightFace;
                    }
                    else
                    {
                        anStack.back() = nLeft;
                        anStack.push_back( nRight );
                        break;
                    }
                }
            }
        }

        void PredictionDegree( int nCorner, std::vector<int> aanStacks[ 3 ], std::vector<int> &anDegree )
        {
            const SCornerTable &t = *pTable;
            int                 nBest = 0;

            aanStacks[ 0 ].push_back( nCorner );

            int nNext = t.Vertex( Next( nCorner ) );
            int nPrev = t.Vertex( Previous( nCorner ) );
            int nTip = t.Vertex( nCorner );

            if ( nNext < 0 || nPrev < 0 || nTip < 0 )
                throw SFail{ __LINE__ };
            if ( ! abVertexVisited[ nNext ] )
            {
                abVertexVisited[ nNext ] = 1;
                Visit( nNext, Next( nCorner ) );
            }
            if ( ! abVertexVisited[ nPrev ] )
            {
                abVertexVisited[ nPrev ] = 1;
                Visit( nPrev, Previous( nCorner ) );
            }
            if ( ! abVertexVisited[ nTip ] )
            {
                abVertexVisited[ nTip ] = 1;
                Visit( nTip, nCorner );
            }

            auto Priority = [ & ]( int c ) -> int
            {
                int v = t.Vertex( c );
                int nPriority = 0;

                if ( v < 0 )
                    throw SFail{ __LINE__ };
                if ( ! abVertexVisited[ v ] )
                {
                    int nDegree = ++anDegree[ v ];

                    nPriority = ( nDegree > 1 ) ? 1 : 2;
                }
                return ( nPriority >= 3 ) ? 2 : nPriority;
            };
            auto Pop = [ & ]() -> int
            {
                for ( int i = nBest; i < 3; i++ )
                {
                    if ( ! aanStacks[ i ].empty() )
                    {
                        int c = aanStacks[ i ].back();

                        aanStacks[ i ].pop_back();
                        nBest = i;
                        return c;
                    }
                }
                return INVALID;
            };

            while ( ( nCorner = Pop() ) >= 0 )
            {
                if ( FaceVisited( nCorner / 3 ) )
                    continue;
                while ( true )
                {
                    abFaceVisited[ nCorner / 3 ] = 1;

                    int v = t.Vertex( nCorner );

                    if ( v < 0 )
                        throw SFail{ __LINE__ };
                    if ( ! abVertexVisited[ v ] )
                    {
                        abVertexVisited[ v ] = 1;
                        Visit( v, nCorner );
                    }

                    int  nRight = t.RightCorner( nCorner );
                    int  nLeft = t.LeftCorner( nCorner );
                    bool bRightVisited = FaceVisited( ( nRight < 0 ) ? -1 : nRight / 3 );
                    bool bLeftVisited = FaceVisited( ( nLeft < 0 ) ? -1 : nLeft / 3 );

                    if ( ! bLeftVisited )
                    {
                        int nPriority = Priority( nLeft );

                        if ( bRightVisited && nPriority <= nBest )
                        {
                            nCorner = nLeft;
                            continue;
                        }
                        aanStacks[ nPriority ].push_back( nLeft );
                        if ( nPriority < nBest )
                            nBest = nPriority;
                    }
                    if ( ! bRightVisited )
                    {
                        int nPriority = Priority( nRight );

                        if ( nPriority <= nBest )
                        {
                            nCorner = nRight;
                            continue;
                        }
                        aanStacks[ nPriority ].push_back( nRight );
                        if ( nPriority < nBest )
                            nBest = nPriority;
                    }
                    break;
                }
            }
        }
    };

        // The octahedral tools for normals.
    struct SOctahedron
    {
        int32_t nMaxQuantized = 0;
        int32_t nMaxValue = 0;
        int32_t nCenter = 0;

        void SetBits( int nBits )
        {
            if ( nBits < 2 || nBits > 30 )
                throw SFail{ __LINE__ };
            nMaxQuantized = ( 1 << nBits ) - 1;
            nMaxValue = nMaxQuantized - 1;
            nCenter = nMaxValue / 2;
        }
        int32_t ModMax( int32_t x ) const
        {
            if ( x > nCenter ) return x - nMaxQuantized;
            if ( x < -nCenter ) return x + nMaxQuantized;
            return x;
        }
        void InvertDiamond( int32_t &s, int32_t &t ) const
        {
            int32_t nSignS, nSignT;

            if ( s >= 0 && t >= 0 ) { nSignS = 1; nSignT = 1; }
            else if ( s <= 0 && t <= 0 ) { nSignS = -1; nSignT = -1; }
            else { nSignS = ( s > 0 ) ? 1 : -1; nSignT = ( t > 0 ) ? 1 : -1; }

            uint32_t nCornerS = (uint32_t)( nSignS * nCenter );
            uint32_t nCornerT = (uint32_t)( nSignT * nCenter );
            uint32_t us = (uint32_t)s;
            uint32_t ut = (uint32_t)t;

            us = us + us - nCornerS;
            ut = ut + ut - nCornerT;
            if ( nSignS * nSignT >= 0 )
            {
                uint32_t nTemp = us;

                us = 0u - ut;
                ut = 0u - nTemp;
            }
            else
            {
                uint32_t nTemp = us;

                us = ut;
                ut = nTemp;
            }
            us = us + nCornerS;
            ut = ut + nCornerT;
            s = (int32_t)us / 2;
            t = (int32_t)ut / 2;
        }
        static void Rotate( int32_t &s, int32_t &t, int nCount )
        {
            int32_t a = s, b = t;

            switch ( nCount )
            {
                case 1: s = b; t = -a; break;
                case 2: s = -a; t = -b; break;
                case 3: s = -b; t = a; break;
                default: break;
            }
        }
        static int RotationCount( int32_t x, int32_t y )
        {
            if ( x == 0 )
                return ( y == 0 ) ? 0 : ( ( y > 0 ) ? 3 : 1 );
            if ( x > 0 )
                return ( y >= 0 ) ? 2 : 1;
            return ( y <= 0 ) ? 0 : 3;
        }
            // The value from a prediction and a correction.
        void Original( const int32_t *pPred, const int32_t *pCorr, int32_t *pOut, bool bCanonical ) const
        {
            int32_t ps = pPred[ 0 ] - nCenter;
            int32_t pt = pPred[ 1 ] - nCenter;
            bool    bInDiamond = std::abs( ps ) + std::abs( pt ) <= nCenter;

            if ( ! bInDiamond )
                InvertDiamond( ps, pt );

            bool bBottomLeft = ( ps == 0 && pt == 0 ) || ( ps < 0 && pt <= 0 );
            int  nRotation = RotationCount( ps, pt );

            if ( bCanonical && ! bBottomLeft )
                Rotate( ps, pt, nRotation );

            int32_t os = ModMax( (int32_t)( (uint32_t)ps + (uint32_t)pCorr[ 0 ] ) );
            int32_t ot = ModMax( (int32_t)( (uint32_t)pt + (uint32_t)pCorr[ 1 ] ) );

            if ( bCanonical && ! bBottomLeft )
                Rotate( os, ot, ( 4 - nRotation ) % 4 );
            if ( ! bInDiamond )
                InvertDiamond( os, ot );
            pOut[ 0 ] = os + nCenter;
            pOut[ 1 ] = ot + nCenter;
        }
        void Canonicalize( int64_t v[ 3 ] ) const
        {
            int64_t nSum = std::llabs( v[ 0 ] ) + std::llabs( v[ 1 ] ) + std::llabs( v[ 2 ] );

            if ( nSum == 0 )
            {
                v[ 0 ] = nCenter;
                return;
            }
            v[ 0 ] = ( v[ 0 ] * nCenter ) / nSum;
            v[ 1 ] = ( v[ 1 ] * nCenter ) / nSum;
            if ( v[ 2 ] >= 0 )
                v[ 2 ] = nCenter - std::llabs( v[ 0 ] ) - std::llabs( v[ 1 ] );
            else
                v[ 2 ] = -( nCenter - std::llabs( v[ 0 ] ) - std::llabs( v[ 1 ] ) );
        }
        void ToCoords( const int64_t v[ 3 ], int32_t &os, int32_t &ot ) const
        {
            int32_t s, t;

            if ( v[ 0 ] >= 0 )
            {
                s = (int32_t)( v[ 1 ] + nCenter );
                t = (int32_t)( v[ 2 ] + nCenter );
            }
            else
            {
                s = ( v[ 1 ] < 0 ) ? (int32_t)std::llabs( v[ 2 ] ) : (int32_t)( nMaxValue - std::llabs( v[ 2 ] ) );
                t = ( v[ 2 ] < 0 ) ? (int32_t)std::llabs( v[ 1 ] ) : (int32_t)( nMaxValue - std::llabs( v[ 1 ] ) );
            }
            if ( ( s == 0 && t == 0 ) || ( s == 0 && t == nMaxValue ) || ( s == nMaxValue && t == 0 ) )
            {
                s = nMaxValue;
                t = nMaxValue;
            }
            else if ( s == 0 && t > nCenter )
                t = nCenter - ( t - nCenter );
            else if ( s == nMaxValue && t < nCenter )
                t = nCenter + ( nCenter - t );
            else if ( t == nMaxValue && s < nCenter )
                s = nCenter + ( nCenter - s );
            else if ( t == 0 && s > nCenter )
                s = nCenter - ( s - nCenter );
            os = s;
            ot = t;
        }
            // Quantized octahedral coordinates to a unit vector.
        void ToVector( int32_t s, int32_t t, float *pOut ) const
        {
            float fScale = 2.0f / (float)nMaxValue;
            float y = (float)s * fScale - 1.0f;
            float z = (float)t * fScale - 1.0f;
            float x = 1.0f - std::fabs( y ) - std::fabs( z );
            float fOffset = -x;

            if ( fOffset < 0.0f )
                fOffset = 0.0f;
            y += ( y < 0.0f ) ? fOffset : -fOffset;
            z += ( z < 0.0f ) ? fOffset : -fOffset;

            float fNorm = x * x + y * y + z * z;

            if ( fNorm < 1e-6f )
            {
                pOut[ 0 ] = pOut[ 1 ] = pOut[ 2 ] = 0.0f;
                return;
            }

            float d = 1.0f / std::sqrt( fNorm );

            pOut[ 0 ] = x * d;
            pOut[ 1 ] = y * d;
            pOut[ 2 ] = z * d;
        }
    };

    struct SAttributeData
    {
        int                 nType = 0;
        int                 nDataType = 0;
        int                 nComponents = 0;
        int                 nUniqueId = 0;
        int                 nDecoderType = 0;       // DECODER_*
        std::vector<int32_t> anPortable;            // the integer values (nPortableComponents a value)
        int                 nPortableComponents = 0;
        std::vector<float>  afValues;
        int                 nDecoder = 0;           // which attribute decoder it is in
    };

    class CDecoder
    {
        public:
            CDecoder( const unsigned char *pData, size_t nSize ) : m_oReader( pData, nSize ) {}
            void Decode( Draco::SMesh &oMesh, std::string &strError );

        private:
            CReader                     m_oReader;
            int                         m_nMethod = 0;
            int                         m_nFaces = 0;
            SCornerTable                m_oTable;           // positions
            std::vector<char>           m_abHole;
                // Attribute connectivity (seams), one for each attribute data.
            struct SSeams
            {
                std::vector<char>   abSeam;             // by corner
                std::vector<char>   abVertexOnSeam;     // by position vertex
                SCornerTable        oTable;
                CRansBits           oBits;
            };
            std::vector<std::unique_ptr<SSeams> > m_apSeams;
            std::vector<SAttributeData> m_aoAttributes;
            int                         m_nPositionAttribute = -1;
            std::vector<int>            m_anPositionCornerValue;   // the position value at each corner
            std::vector<std::vector<int> > m_aoCornerValues;       // each attribute's value at each corner

            void Connectivity( std::string &strError );
            void Sequential( void );
            void EdgeBreaker( void );
            void Attributes( void );
            void DecodeValues( SAttributeData &oAttribute, const STraversal &oTraversal, int nValues );
            const int32_t *PositionAt( int nCorner ) const;
    };

    void CDecoder::Sequential( void )
    {
        uint32_t nFaces = m_oReader.Var32();
        uint32_t nPoints = m_oReader.Var32();
        uint8_t  nMethod = m_oReader.U8();

        if ( nFaces > ( 1u << 28 ) || nPoints > ( 1u << 28 ) )
            throw SFail{ __LINE__ };
        m_nFaces = (int)nFaces;
        m_oTable.anVertex.assign( (size_t)nFaces * 3, 0 );
        if ( nMethod == 0 )
        {
            std::vector<uint32_t> anSymbols;
            int32_t               nLast = 0;

            DecodeSymbols( m_oReader, nFaces * 3, 1, anSymbols );
            for ( size_t i = 0; i < anSymbols.size(); i++ )
            {
                int32_t nDiff = (int32_t)( anSymbols[ i ] >> 1 );

                if ( anSymbols[ i ] & 1 )
                {
                    if ( nDiff > nLast )
                        throw SFail{ __LINE__ };
                    nDiff = -nDiff;
                }
                nLast += nDiff;
                if ( nLast < 0 || (uint32_t)nLast >= nPoints )
                    throw SFail{ __LINE__ };
                m_oTable.anVertex[ i ] = nLast;
            }
        }
        else if ( nMethod == 1 )
        {
            for ( size_t i = 0; i < (size_t)nFaces * 3; i++ )
            {
                uint32_t nIndex;

                if ( nPoints < 256 ) nIndex = m_oReader.U8();
                else if ( nPoints < ( 1u << 16 ) ) { nIndex = m_oReader.U8(); nIndex |= (uint32_t)m_oReader.U8() << 8; }
                else if ( nPoints < ( 1u << 21 ) ) nIndex = m_oReader.Var32();
                else nIndex = m_oReader.U32();
                if ( nIndex >= nPoints )
                    throw SFail{ __LINE__ };
                m_oTable.anVertex[ i ] = (int)nIndex;
            }
        }
        else
            throw SFail{ __LINE__ };
        m_oTable.anLeftMost.assign( nPoints, INVALID );
    }

    void CDecoder::EdgeBreaker( void )
    {
        CReader &r = m_oReader;
        int      nTraversal = r.U8();
        uint32_t nVertices = r.Var32();
        uint32_t nFaces = r.Var32();
        int      nAttributeData = r.U8();
        uint32_t nSymbols = r.Var32();
        uint32_t nSplitSymbols = r.Var32();

        if ( nFaces > ( 1u << 28 ) || nVertices > ( 1u << 28 ) || nSymbols > nFaces || nSplitSymbols > nFaces )
            throw SFail{ __LINE__ };
        if ( nTraversal != EDGEBREAKER_STANDARD && nTraversal != EDGEBREAKER_VALENCE )
            throw SFail{ __LINE__ };
        m_nFaces = (int)nFaces;

            // Topology splits.
        uint32_t nSplits = r.Var32();
        std::vector<int> anSource( nSplits ), anSplit( nSplits ), anEdge( nSplits );

        if ( nSplits > nFaces )
            throw SFail{ __LINE__ };
        {
            int nLast = 0;

            for ( uint32_t i = 0; i < nSplits; i++ )
            {
                uint32_t nDelta = r.Var32();

                anSource[ i ] = (int)nDelta + nLast;
                nDelta = r.Var32();
                if ( (int)nDelta > anSource[ i ] )
                    throw SFail{ __LINE__ };
                anSplit[ i ] = anSource[ i ] - (int)nDelta;
                nLast = anSource[ i ];
            }
            if ( nSplits > 0 )
            {
                r.StartBits();
                for ( uint32_t i = 0; i < nSplits; i++ )
                    anEdge[ i ] = (int)r.Bits( 1 );
                r.EndBits();
            }
        }

            // The traversal data.
        CReader     oSymbolBits( r );
        CRansBits   oStartFaces;
        std::vector<std::vector<uint32_t> > aanContext;
        std::vector<int> anContextCount;

        if ( nTraversal == EDGEBREAKER_STANDARD )
        {
            uint64_t nBytes = r.Var();

            if ( nBytes > r.Left() )
                throw SFail{ __LINE__ };
            oSymbolBits = r;
            oSymbolBits.StartBits();
            r.Skip( nBytes );
        }
        oStartFaces.Start( r );
        m_apSeams.clear();
        for ( int i = 0; i < nAttributeData; i++ )
        {
            m_apSeams.push_back( std::unique_ptr<SSeams>( new SSeams() ) );
            m_apSeams.back()->oBits.Start( r );
        }
        if ( nTraversal == EDGEBREAKER_VALENCE )
        {
            int8_t nMode = r.I8();

            if ( nMode != 0 )
                throw SFail{ __LINE__ };
            aanContext.resize( 6 );
            anContextCount.assign( 6, 0 );
            for ( int i = 0; i < 6; i++ )
            {
                uint32_t nCount = r.Var32();

                if ( nCount > nFaces )
                    throw SFail{ __LINE__ };
                if ( nCount > 0 )
                {
                    DecodeSymbols( r, nCount, 1, aanContext[ i ] );
                    anContextCount[ i ] = (int)nCount;
                }
            }
        }

            // Connectivity.
        size_t nMaxVertices = (size_t)nVertices + nSplitSymbols;
        SCornerTable &t = m_oTable;

        t.anVertex.assign( (size_t)nFaces * 3, INVALID );
        t.anOpposite.assign( (size_t)nFaces * 3, INVALID );
        t.anLeftMost.clear();
        m_abHole.assign( nMaxVertices, 1 );

        std::vector<int>        anActive;
        std::map<int, int>      oSplitCorners;
        std::vector<int>        anValence;
        int                     nContext = -1;
        int                     nLastSymbol = -1;
        int                     nFace = 0;
        size_t                  nNextSplit = nSplits;   // splits are taken from the back

        if ( nTraversal == EDGEBREAKER_VALENCE )
            anValence.assign( nMaxVertices, 0 );

        auto NewVertex = [ & ]() -> int
        {
            if ( t.anLeftMost.size() >= nMaxVertices )
                throw SFail{ __LINE__ };
            t.anLeftMost.push_back( INVALID );
            return (int)t.anLeftMost.size() - 1;
        };
        auto Map = [ & ]( int c, int v ) { t.anVertex[ c ] = v; if ( v >= 0 ) t.anLeftMost[ v ] = c; };
        auto SetOpposite = [ & ]( int a, int b ) { t.anOpposite[ a ] = b; t.anOpposite[ b ] = a; };
        static const int s_anSymbolToTopology[ 5 ] = { TOPOLOGY_C, TOPOLOGY_S, TOPOLOGY_L, TOPOLOGY_R, TOPOLOGY_E };

        for ( uint32_t nSymbol = 0; nSymbol < nSymbols; nSymbol++ )
        {
            int  c = 3 * nFace;
            bool bCheckSplit = false;
            int  nTopology;

            if ( nTraversal == EDGEBREAKER_STANDARD )
            {
                nTopology = (int)oSymbolBits.Bits( 1 );
                if ( nTopology != TOPOLOGY_C )
                    nTopology |= (int)( oSymbolBits.Bits( 2 ) << 1 );
            }
            else if ( nContext != -1 )
            {
                int nCount = --anContextCount[ nContext ];

                if ( nCount < 0 )
                    throw SFail{ __LINE__ };

                uint32_t nId = aanContext[ nContext ][ nCount ];

                if ( nId > 4 )
                    throw SFail{ __LINE__ };
                nTopology = s_anSymbolToTopology[ nId ];
            }
            else
                nTopology = TOPOLOGY_E;
            nLastSymbol = nTopology;
            nFace++;
            if ( (uint32_t)nFace > nFaces )
                throw SFail{ __LINE__ };

            if ( nTopology == TOPOLOGY_C )
            {
                if ( anActive.empty() )
                    throw SFail{ __LINE__ };

                int a = anActive.back();
                int b = Previous( a );

                while ( t.anOpposite[ b ] >= 0 )
                    b = Previous( t.anOpposite[ b ] );
                if ( t.anOpposite[ a ] >= 0 || a == b )
                    throw SFail{ __LINE__ };
                SetOpposite( a, c + 1 );
                SetOpposite( b, c + 2 );
                anActive.back() = c;

                int x = t.anVertex[ Next( a ) ];
                int n = t.anVertex[ Next( b ) ];
                int p = t.anVertex[ Previous( a ) ];

                if ( x < 0 || n < 0 || p < 0 || x == n || x == p )
                    throw SFail{ __LINE__ };
                Map( c, x );
                Map( c + 1, n );
                Map( c + 2, p );
                m_abHole[ x ] = 0;
            }
            else if ( nTopology == TOPOLOGY_S )
            {
                if ( anActive.empty() )
                    throw SFail{ __LINE__ };

                int b = anActive.back();

                anActive.pop_back();

                auto oFound = oSplitCorners.find( (int)nSymbol );

                if ( oFound != oSplitCorners.end() )
                    anActive.push_back( oFound->second );
                if ( anActive.empty() )
                    throw SFail{ __LINE__ };

                int a = anActive.back();

                if ( a == b || t.anOpposite[ a ] >= 0 || t.anOpposite[ b ] >= 0 )
                    throw SFail{ __LINE__ };
                SetOpposite( a, c + 2 );
                SetOpposite( b, c + 1 );

                int vp = t.anVertex[ Previous( a ) ];

                Map( c, vp );
                Map( c + 1, t.anVertex[ Next( a ) ] );
                Map( c + 2, t.anVertex[ Previous( b ) ] );

                int cn = Next( b );
                int vn = t.anVertex[ cn ];

                if ( vp < 0 || vn < 0 )
                    throw SFail{ __LINE__ };
                if ( nTraversal == EDGEBREAKER_VALENCE )
                    anValence[ vp ] += anValence[ vn ];

                    // Merge vertex n into p: every corner around n, swinging
                    // left from cn.
                int nFirst = cn;

                while ( cn >= 0 )
                {
                    Map( cn, vp );
                    cn = t.SwingLeft( cn );
                    if ( cn == nFirst )
                        throw SFail{ __LINE__ };
                }
                t.anLeftMost[ vn ] = INVALID;
                anActive.back() = c;
            }
            else if ( nTopology == TOPOLOGY_R || nTopology == TOPOLOGY_L )
            {
                if ( anActive.empty() )
                    throw SFail{ __LINE__ };

                int a = anActive.back();

                if ( t.anOpposite[ a ] >= 0 )
                    throw SFail{ __LINE__ };

                int nOpp, nCornerL, nCornerR;

                if ( nTopology == TOPOLOGY_R )
                {
                    nOpp = c + 2;
                    nCornerL = c + 1;
                    nCornerR = c;
                }
                else
                {
                    nOpp = c + 1;
                    nCornerL = c;
                    nCornerR = c + 2;
                }
                SetOpposite( nOpp, a );

                int nNew = NewVertex();
                int vr = t.anVertex[ Previous( a ) ];
                int vl = t.anVertex[ Next( a ) ];

                Map( nOpp, nNew );
                Map( nCornerR, vr );
                Map( nCornerL, vl );
                anActive.back() = c;
                bCheckSplit = true;
            }
            else if ( nTopology == TOPOLOGY_E )
            {
                int v0 = NewVertex();
                int v1 = NewVertex();
                int v2 = NewVertex();

                Map( c, v0 );
                Map( c + 1, v1 );
                Map( c + 2, v2 );
                anActive.push_back( c );
                bCheckSplit = true;
            }
            else
                throw SFail{ __LINE__ };

            if ( nTraversal == EDGEBREAKER_VALENCE )
            {
                int cc = anActive.back();
                int vt = t.anVertex[ cc ], vn = t.anVertex[ Next( cc ) ], vpv = t.anVertex[ Previous( cc ) ];

                if ( vt < 0 || vn < 0 || vpv < 0 )
                    throw SFail{ __LINE__ };
                switch ( nLastSymbol )
                {
                    case TOPOLOGY_C:
                    case TOPOLOGY_S:
                        anValence[ vn ] += 1;
                        anValence[ vpv ] += 1;
                        break;
                    case TOPOLOGY_R:
                        anValence[ vt ] += 1;
                        anValence[ vn ] += 1;
                        anValence[ vpv ] += 2;
                        break;
                    case TOPOLOGY_L:
                        anValence[ vt ] += 1;
                        anValence[ vn ] += 2;
                        anValence[ vpv ] += 1;
                        break;
                    case TOPOLOGY_E:
                        anValence[ vt ] += 2;
                        anValence[ vn ] += 2;
                        anValence[ vpv ] += 2;
                        break;
                    default:
                        break;
                }

                int nValence = anValence[ vn ];

                if ( nValence < 2 ) nValence = 2;
                if ( nValence > 7 ) nValence = 7;
                nContext = nValence - 2;
            }

            if ( bCheckSplit )
            {
                int nEncoderSymbol = (int)nSymbols - (int)nSymbol - 1;

                while ( nNextSplit > 0 && anSource[ nNextSplit - 1 ] == nEncoderSymbol )
                {
                    nNextSplit--;

                    int nTop = anActive.back();
                    int nNewActive = ( anEdge[ nNextSplit ] == 1 ) ? Next( nTop ) : Previous( nTop );
                    int nDecoderSplit = (int)nSymbols - anSplit[ nNextSplit ] - 1;

                    oSplitCorners[ nDecoderSplit ] = nNewActive;
                }
            }
        }

            // Start faces: each open edge left on the stack either closes
            // an interior face or starts at a boundary.
        while ( ! anActive.empty() )
        {
            int a = anActive.back();

            anActive.pop_back();
            if ( oStartFaces.Next() )
            {
                if ( (uint32_t)nFace >= nFaces )
                    throw SFail{ __LINE__ };

                int b = Previous( a );

                while ( t.anOpposite[ b ] >= 0 )
                    b = Previous( t.anOpposite[ b ] );

                int cc = Next( a );

                while ( t.anOpposite[ cc ] >= 0 )
                    cc = Next( t.anOpposite[ cc ] );
                if ( a == b || a == cc || b == cc || t.anOpposite[ a ] >= 0 )
                    throw SFail{ __LINE__ };

                int vn = t.anVertex[ Next( a ) ];
                int vx = t.anVertex[ Next( b ) ];
                int vp = t.anVertex[ Next( cc ) ];

                if ( vn < 0 || vx < 0 || vp < 0 )
                    throw SFail{ __LINE__ };

                int nc = 3 * nFace++;

                SetOpposite( nc, a );
                SetOpposite( nc + 1, b );
                SetOpposite( nc + 2, cc );
                Map( nc, vx );
                Map( nc + 1, vp );
                Map( nc + 2, vn );
                m_abHole[ vx ] = 0;
                m_abHole[ vp ] = 0;
                m_abHole[ vn ] = 0;
            }
        }
        if ( (uint32_t)nFace != nFaces )
            throw SFail{ __LINE__ };
        for ( size_t c = 0; c < t.anVertex.size(); c++ )
        {
            if ( t.anVertex[ c ] < 0 )
                throw SFail{ __LINE__ };
        }

            // A boundary vertex's left-most corner is its corner that can't
            // swing further left.
        for ( int v = 0; v < t.Vertices(); v++ )
        {
            if ( ! m_abHole[ v ] )
                continue;

            int nFirst = t.anLeftMost[ v ];

            if ( nFirst < 0 )
                continue;

            int c = nFirst;
            int nLeft = t.SwingLeft( nFirst );

            while ( nLeft >= 0 && nLeft != nFirst )
            {
                c = nLeft;
                nLeft = t.SwingLeft( nLeft );
            }
            if ( nLeft != nFirst )
                t.anLeftMost[ v ] = c;
        }

            // Attribute seams, for each edge of each face once.
        for ( auto &pSeams : m_apSeams )
        {
            pSeams->abSeam.assign( t.anVertex.size(), 0 );
            pSeams->abVertexOnSeam.assign( (size_t)t.Vertices(), 0 );
        }
        if ( ! m_apSeams.empty() )
        {
            for ( int f = 0; f < (int)nFaces; f++ )
            {
                for ( int k = 0; k < 3; k++ )
                {
                    int c = 3 * f + k;
                    int o = t.anOpposite[ c ];

                    for ( size_t i = 0; i < m_apSeams.size(); i++ )
                    {
                        SSeams &oSeams = *m_apSeams[ i ];
                        bool    bSeam;

                        if ( o < 0 )
                            bSeam = true;
                        else if ( o / 3 < f )
                            continue;
                        else
                            bSeam = oSeams.oBits.Next();
                        if ( ! bSeam )
                            continue;
                        oSeams.abSeam[ c ] = 1;
                        oSeams.abVertexOnSeam[ t.anVertex[ Next( c ) ] ] = 1;
                        oSeams.abVertexOnSeam[ t.anVertex[ Previous( c ) ] ] = 1;
                        if ( o >= 0 )
                        {
                            oSeams.abSeam[ o ] = 1;
                            oSeams.abVertexOnSeam[ t.anVertex[ Next( o ) ] ] = 1;
                            oSeams.abVertexOnSeam[ t.anVertex[ Previous( o ) ] ] = 1;
                        }
                    }
                }
            }

                // Each attribute's own vertices: a position vertex split
                // wherever a seam crosses its fan.
            for ( auto &pSeams : m_apSeams )
            {
                SCornerTable &a = pSeams->oTable;

                a.anOpposite = t.anOpposite;
                a.pSeam = &pSeams->abSeam;
                a.anVertex.assign( t.anVertex.size(), INVALID );
                a.anLeftMost.clear();
                for ( int v = 0; v < t.Vertices(); v++ )
                {
                    int c = t.anLeftMost[ v ];

                    if ( c < 0 )
                        continue;

                    int nId = (int)a.anLeftMost.size();
                    int nFirst = c;

                    if ( pSeams->abVertexOnSeam[ v ] )
                    {
                        int nAct = a.SwingLeft( nFirst );

                        while ( nAct >= 0 )
                        {
                            nFirst = nAct;
                            nAct = a.SwingLeft( nAct );
                            if ( nAct == c )
                                throw SFail{ __LINE__ };
                        }
                    }
                    a.anVertex[ nFirst ] = nId;
                    a.anLeftMost.push_back( nFirst );

                    int nAct = t.SwingRight( nFirst );

                    while ( nAct >= 0 && nAct != nFirst )
                    {
                        if ( pSeams->abSeam[ Next( nAct ) ] )
                        {
                            nId = (int)a.anLeftMost.size();
                            a.anLeftMost.push_back( nAct );
                        }
                        a.anVertex[ nAct ] = nId;
                        nAct = t.SwingRight( nAct );
                    }
                }
                for ( size_t c = 0; c < a.anVertex.size(); c++ )
                {
                    if ( a.anVertex[ c ] < 0 )
                        throw SFail{ __LINE__ };
                }
            }
        }
    }

    void CDecoder::Connectivity( std::string &strError )
    {
        (void)strError;
        if ( m_nMethod == MESH_SEQUENTIAL )
            Sequential();
        else
            EdgeBreaker();
    }

    const int32_t *CDecoder::PositionAt( int nCorner ) const
    {
        if ( m_nPositionAttribute < 0 || m_anPositionCornerValue.empty() || nCorner < 0 ||
             (size_t)nCorner >= m_anPositionCornerValue.size() )
            throw SFail{ __LINE__ };

        const SAttributeData &oPositions = m_aoAttributes[ (size_t)m_nPositionAttribute ];
        int                   nValue = m_anPositionCornerValue[ (size_t)nCorner ];

        if ( nValue < 0 || (size_t)( nValue + 1 ) * 3 > oPositions.anPortable.size() )
            throw SFail{ __LINE__ };
        return &oPositions.anPortable[ (size_t)nValue * 3 ];
    }

    void CDecoder::DecodeValues( SAttributeData &oAttribute, const STraversal &oTraversal, int nValues )
    {
        CReader &r = m_oReader;
        int      nComponents = oAttribute.nPortableComponents;
        int8_t   nPrediction = r.I8();
        int8_t   nTransform = 0;

        if ( nPrediction != PREDICTION_NONE )
        {
            nTransform = r.I8();
            if ( nPrediction != PREDICTION_DIFFERENCE && nPrediction != PREDICTION_PARALLELOGRAM &&
                 nPrediction != PREDICTION_CONSTRAINED_MULTI && nPrediction != PREDICTION_TEX_COORDS_PORTABLE &&
                 nPrediction != PREDICTION_GEOMETRIC_NORMAL )
                throw SFail{ __LINE__ };
            if ( nTransform != TRANSFORM_WRAP && nTransform != TRANSFORM_OCTAHEDRON && nTransform != TRANSFORM_OCTAHEDRON_CANONICALIZED )
                throw SFail{ __LINE__ };
            if ( nPrediction != PREDICTION_DIFFERENCE && m_nMethod != MESH_EDGEBREAKER )
                throw SFail{ __LINE__ };
        }

        size_t                 nTotal = (size_t)nValues * (size_t)nComponents;
        std::vector<uint32_t>  anRaw;
        uint8_t                nCompressed = r.U8();

        if ( nCompressed > 0 )
            DecodeSymbols( r, (uint32_t)nTotal, nComponents, anRaw );
        else
        {
            uint8_t nBytes = r.U8();

            if ( nBytes < 1 || nBytes > 4 )
                throw SFail{ __LINE__ };
            anRaw.assign( nTotal, 0 );
            for ( size_t i = 0; i < nTotal; i++ )
            {
                uint32_t n = 0;

                for ( int b = 0; b < nBytes; b++ )
                    n |= (uint32_t)r.U8() << ( 8 * b );
                anRaw[ i ] = n;
            }
        }

        std::vector<int32_t> &anOut = oAttribute.anPortable;
        std::vector<int32_t>  anCorr( nTotal );
        bool                  bOctahedral = ( nPrediction != PREDICTION_NONE &&
                                              ( nTransform == TRANSFORM_OCTAHEDRON || nTransform == TRANSFORM_OCTAHEDRON_CANONICALIZED ) );

        for ( size_t i = 0; i < nTotal; i++ )
        {
            if ( bOctahedral )
                anCorr[ i ] = (int32_t)anRaw[ i ];
            else
            {
                uint32_t n = anRaw[ i ];

                anCorr[ i ] = ( n & 1 ) ? -(int32_t)( n >> 1 ) - 1 : (int32_t)( n >> 1 );
            }
        }
        anOut = anCorr;
        if ( nPrediction == PREDICTION_NONE )
            return;

            // The prediction scheme's own data.
        std::vector<std::vector<char> > aabCrease( MAX_PARALLELOGRAMS );
        std::vector<char>               abOrientation;
        CRansBits                       oFlips;
        int32_t                         nWrapMin = 0, nWrapMax = 0;
        SOctahedron                     oOctahedron;

        auto TransformData = [ & ]()
        {
            if ( nTransform == TRANSFORM_WRAP )
            {
                nWrapMin = r.I32();
                nWrapMax = r.I32();
                if ( nWrapMin > nWrapMax )
                    throw SFail{ __LINE__ };
            }
            else
            {
                int32_t nMax = r.I32();

                r.I32();    // the center value, which follows from the maximum

                if ( nMax % 2 == 0 || nMax <= 0 )
                    throw SFail{ __LINE__ };

                int nBits = 0;

                while ( ( nMax >> nBits ) != 0 )
                    nBits++;
                oOctahedron.SetBits( nBits );
            }
        };
        if ( nPrediction == PREDICTION_CONSTRAINED_MULTI )
        {
            for ( int i = 0; i < MAX_PARALLELOGRAMS; i++ )
            {
                uint32_t nFlags = r.Var32();

                if ( nFlags > (uint32_t)( m_nFaces * 3 ) )
                    throw SFail{ __LINE__ };
                if ( nFlags > 0 )
                {
                    CRansBits oBits;

                    oBits.Start( r );
                    aabCrease[ i ].resize( nFlags );
                    for ( uint32_t j = 0; j < nFlags; j++ )
                        aabCrease[ i ][ j ] = oBits.Next() ? 1 : 0;
                }
            }
            TransformData();
        }
        else if ( nPrediction == PREDICTION_TEX_COORDS_PORTABLE )
        {
            int32_t nOrientations = r.I32();

            if ( nOrientations < 0 || nOrientations > m_nFaces * 3 )
                throw SFail{ __LINE__ };

            CRansBits oBits;
            bool      bLast = true;

            oBits.Start( r );
            abOrientation.resize( (size_t)nOrientations );
            for ( int32_t i = 0; i < nOrientations; i++ )
            {
                if ( ! oBits.Next() )
                    bLast = ! bLast;
                abOrientation[ (size_t)i ] = bLast ? 1 : 0;
            }
            TransformData();
        }
        else if ( nPrediction == PREDICTION_GEOMETRIC_NORMAL )
        {
            TransformData();
            oFlips.Start( r );
        }
        else
            TransformData();

        if ( ( nTransform == TRANSFORM_WRAP ) == bOctahedral )
            throw SFail{ __LINE__ };
        if ( bOctahedral && nComponents != 2 )
            throw SFail{ __LINE__ };

        int32_t nWrapRange = nWrapMax - nWrapMin + 1;

            // Transform: from a prediction and a correction to the value.
        auto Original = [ & ]( const int32_t *pPred, const int32_t *pCorr, int32_t *pOut )
        {
            if ( bOctahedral )
            {
                oOctahedron.Original( pPred, pCorr, pOut, nTransform == TRANSFORM_OCTAHEDRON_CANONICALIZED );
                return;
            }
            for ( int i = 0; i < nComponents; i++ )
            {
                int32_t nPred = pPred[ i ];

                if ( nPred > nWrapMax ) nPred = nWrapMax;
                else if ( nPred < nWrapMin ) nPred = nWrapMin;

                int32_t nValue = (int32_t)( (uint32_t)nPred + (uint32_t)pCorr[ i ] );

                if ( nValue > nWrapMax )
                    nValue = (int32_t)( (uint32_t)nValue - (uint32_t)nWrapRange );
                else if ( nValue < nWrapMin )
                    nValue = (int32_t)( (uint32_t)nValue + (uint32_t)nWrapRange );
                pOut[ i ] = nValue;
            }
        };

        const SCornerTable     &t = *oTraversal.pTable;
        const std::vector<int> &anValueCorner = oTraversal.anValueCorner;
        const std::vector<int> &anVertexValue = oTraversal.anVertexValue;
        std::vector<int32_t>    anZero( (size_t)nComponents, 0 );
        int32_t                *pData = anOut.data();

        auto Parallelogram = [ & ]( int nValue, int c, int32_t *pPrediction ) -> bool
        {
            int oc = t.Opposite( c );

            if ( oc < 0 )
                return false;

            int nOpp = anVertexValue[ t.Vertex( oc ) ];
            int nNext = anVertexValue[ t.Vertex( Next( oc ) ) ];
            int nPrev = anVertexValue[ t.Vertex( Previous( oc ) ) ];

            if ( nOpp < nValue && nNext < nValue && nPrev < nValue )
            {
                for ( int k = 0; k < nComponents; k++ )
                {
                    int64_t nResult = (int64_t)pData[ nNext * nComponents + k ] + (int64_t)pData[ nPrev * nComponents + k ] -
                                      (int64_t)pData[ nOpp * nComponents + k ];

                    pPrediction[ k ] = (int32_t)nResult;
                }
                return true;
            }
            return false;
        };

        if ( nValues <= 0 )
            return;

        if ( nPrediction == PREDICTION_DIFFERENCE )
        {
            Original( anZero.data(), &anCorr[ 0 ], &pData[ 0 ] );
            for ( size_t i = (size_t)nComponents; i < nTotal; i += (size_t)nComponents )
                Original( &pData[ i - (size_t)nComponents ], &anCorr[ i ], &pData[ i ] );
        }
        else if ( nPrediction == PREDICTION_PARALLELOGRAM )
        {
            std::vector<int32_t> anPred( (size_t)nComponents );

            Original( anZero.data(), &anCorr[ 0 ], &pData[ 0 ] );
            for ( int p = 1; p < nValues; p++ )
            {
                int    c = anValueCorner[ (size_t)p ];
                size_t nAt = (size_t)p * (size_t)nComponents;

                if ( Parallelogram( p, c, anPred.data() ) )
                    Original( anPred.data(), &anCorr[ nAt ], &pData[ nAt ] );
                else
                    Original( &pData[ nAt - (size_t)nComponents ], &anCorr[ nAt ], &pData[ nAt ] );
            }
        }
        else if ( nPrediction == PREDICTION_CONSTRAINED_MULTI )
        {
            std::vector<std::vector<int32_t> > aanPred( MAX_PARALLELOGRAMS, std::vector<int32_t>( (size_t)nComponents, 0 ) );
            std::vector<int32_t>                anMulti( (size_t)nComponents );
            std::vector<size_t>                 anCreaseAt( MAX_PARALLELOGRAMS, 0 );

            Original( aanPred[ 0 ].data(), &anCorr[ 0 ], &pData[ 0 ] );
            for ( int p = 1; p < nValues; p++ )
            {
                int  nStart = anValueCorner[ (size_t)p ];
                int  c = nStart;
                int  nCount = 0;
                bool bFirstPass = true;

                while ( c >= 0 )
                {
                    if ( Parallelogram( p, c, aanPred[ (size_t)nCount ].data() ) )
                    {
                        if ( ++nCount == MAX_PARALLELOGRAMS )
                            break;
                    }
                    c = bFirstPass ? t.SwingLeft( c ) : t.SwingRight( c );
                    if ( c == nStart )
                        break;
                    if ( c < 0 && bFirstPass )
                    {
                        bFirstPass = false;
                        c = t.SwingRight( nStart );
                    }
                }

                int nUsed = 0;

                if ( nCount > 0 )
                {
                    for ( int k = 0; k < nComponents; k++ )
                        anMulti[ (size_t)k ] = 0;
                    for ( int i = 0; i < nCount; i++ )
                    {
                        int    nContextIndex = nCount - 1;
                        size_t nAt = anCreaseAt[ (size_t)nContextIndex ]++;

                        if ( nAt >= aabCrease[ (size_t)nContextIndex ].size() )
                            throw SFail{ __LINE__ };
                        if ( ! aabCrease[ (size_t)nContextIndex ][ nAt ] )
                        {
                            nUsed++;
                            for ( int k = 0; k < nComponents; k++ )
                                anMulti[ (size_t)k ] = (int32_t)( (uint32_t)anMulti[ (size_t)k ] + (uint32_t)aanPred[ (size_t)i ][ (size_t)k ] );
                        }
                    }
                }

                size_t nAt = (size_t)p * (size_t)nComponents;

                if ( nUsed == 0 )
                    Original( &pData[ nAt - (size_t)nComponents ], &anCorr[ nAt ], &pData[ nAt ] );
                else
                {
                    for ( int k = 0; k < nComponents; k++ )
                        anMulti[ (size_t)k ] /= nUsed;
                    Original( anMulti.data(), &anCorr[ nAt ], &pData[ nAt ] );
                }
            }
        }
        else if ( nPrediction == PREDICTION_TEX_COORDS_PORTABLE )
        {
            if ( nComponents != 2 || m_nPositionAttribute < 0 )
                throw SFail{ __LINE__ };
            for ( int p = 0; p < nValues; p++ )
            {
                int     c = anValueCorner[ (size_t)p ];
                int     nNextValue = anVertexValue[ t.Vertex( Next( c ) ) ];
                int     nPrevValue = anVertexValue[ t.Vertex( Previous( c ) ) ];
                int32_t anPredicted[ 2 ] = { 0, 0 };
                bool    bDone = false;

                if ( nPrevValue < p && nNextValue < p )
                {
                    int64_t nU = pData[ nNextValue * 2 ], nV = pData[ nNextValue * 2 + 1 ];
                    int64_t pU = pData[ nPrevValue * 2 ], pV = pData[ nPrevValue * 2 + 1 ];

                    if ( nU == pU && nV == pV )
                    {
                        anPredicted[ 0 ] = (int32_t)pU;
                        anPredicted[ 1 ] = (int32_t)pV;
                        bDone = true;
                    }
                    else
                    {
                        const int32_t *pTip = PositionAt( anValueCorner[ (size_t)p ] );
                        const int32_t *pNext = PositionAt( anValueCorner[ (size_t)nNextValue ] );
                        const int32_t *pPrev = PositionAt( anValueCorner[ (size_t)nPrevValue ] );
                        int64_t        pn[ 3 ], cn[ 3 ];

                        for ( int k = 0; k < 3; k++ )
                        {
                            pn[ k ] = (int64_t)pPrev[ k ] - pNext[ k ];
                            cn[ k ] = (int64_t)pTip[ k ] - pNext[ k ];
                        }

                        uint64_t nPnSquared = (uint64_t)( pn[ 0 ] * pn[ 0 ] + pn[ 1 ] * pn[ 1 ] + pn[ 2 ] * pn[ 2 ] );

                        if ( nPnSquared != 0 )
                        {
                            int64_t nCnDotPn = pn[ 0 ] * cn[ 0 ] + pn[ 1 ] * cn[ 1 ] + pn[ 2 ] * cn[ 2 ];
                            int64_t nPnUvU = pU - nU, nPnUvV = pV - nV;
                            int64_t nAbsMax = (std::max)( (std::max)( std::llabs( pn[ 0 ] ), std::llabs( pn[ 1 ] ) ), std::llabs( pn[ 2 ] ) );

                            if ( nAbsMax > 0 && nCnDotPn > (std::numeric_limits<int64_t>::max)() / nAbsMax )
                                throw SFail{ __LINE__ };

                            int64_t nXu = nU * (int64_t)nPnSquared + nCnDotPn * nPnUvU;
                            int64_t nXv = nV * (int64_t)nPnSquared + nCnDotPn * nPnUvV;
                            int64_t x[ 3 ], cx[ 3 ];

                            for ( int k = 0; k < 3; k++ )
                            {
                                x[ k ] = pNext[ k ] + ( nCnDotPn * pn[ k ] ) / (int64_t)nPnSquared;
                                cx[ k ] = pTip[ k ] - x[ k ];
                            }

                            uint64_t nCxSquared = (uint64_t)( cx[ 0 ] * cx[ 0 ] + cx[ 1 ] * cx[ 1 ] + cx[ 2 ] * cx[ 2 ] );
                            uint64_t nProduct = nCxSquared * nPnSquared;
                            uint64_t nRoot = 0;

                            if ( nProduct != 0 )
                            {
                                uint64_t nAct = nProduct;

                                nRoot = 1;
                                while ( nAct >= 2 )
                                {
                                    nRoot *= 2;
                                    nAct /= 4;
                                }
                                do
                                {
                                    nRoot = ( nRoot + nProduct / nRoot ) / 2;
                                }
                                while ( nRoot * nRoot > nProduct );
                            }

                            int64_t nCxU = nPnUvV * (int64_t)nRoot;
                            int64_t nCxV = -nPnUvU * (int64_t)nRoot;

                            if ( abOrientation.empty() )
                                throw SFail{ __LINE__ };

                            bool bOrientation = abOrientation.back() != 0;

                            abOrientation.pop_back();

                            int64_t nPu, nPv;

                            if ( bOrientation )
                            {
                                nPu = (int64_t)( (uint64_t)nXu + (uint64_t)nCxU ) / (int64_t)nPnSquared;
                                nPv = (int64_t)( (uint64_t)nXv + (uint64_t)nCxV ) / (int64_t)nPnSquared;
                            }
                            else
                            {
                                nPu = (int64_t)( (uint64_t)nXu - (uint64_t)nCxU ) / (int64_t)nPnSquared;
                                nPv = (int64_t)( (uint64_t)nXv - (uint64_t)nCxV ) / (int64_t)nPnSquared;
                            }
                            anPredicted[ 0 ] = (int32_t)nPu;
                            anPredicted[ 1 ] = (int32_t)nPv;
                            bDone = true;
                        }
                    }
                }
                if ( ! bDone )
                {
                    int nFrom = -1;

                    if ( nPrevValue < p )
                        nFrom = nPrevValue;
                    if ( nNextValue < p )
                        nFrom = nNextValue;
                    else if ( p > 0 )
                        nFrom = p - 1;
                    else
                        nFrom = -1;
                    if ( nFrom >= 0 )
                    {
                        anPredicted[ 0 ] = pData[ nFrom * 2 ];
                        anPredicted[ 1 ] = pData[ nFrom * 2 + 1 ];
                    }
                }
                Original( anPredicted, &anCorr[ (size_t)p * 2 ], &pData[ (size_t)p * 2 ] );
            }
        }
        else if ( nPrediction == PREDICTION_GEOMETRIC_NORMAL )
        {
            if ( nComponents != 2 || ! bOctahedral || m_nPositionAttribute < 0 )
                throw SFail{ __LINE__ };
            for ( int nValue = 0; nValue < nValues; nValue++ )
            {
                int            nCorner = anValueCorner[ (size_t)nValue ];
                const int32_t *pCenter = PositionAt( nCorner );
                int64_t        anNormal[ 3 ] = { 0, 0, 0 };
                int            c = nCorner;
                bool           bLeft = true;

                while ( c >= 0 )
                {
                    const int32_t *pNext = PositionAt( Next( c ) );
                    const int32_t *pPrev = PositionAt( Previous( c ) );
                    int64_t        dn[ 3 ], dp[ 3 ];

                    for ( int k = 0; k < 3; k++ )
                    {
                        dn[ k ] = (int64_t)pNext[ k ] - pCenter[ k ];
                        dp[ k ] = (int64_t)pPrev[ k ] - pCenter[ k ];
                    }
                    anNormal[ 0 ] = (int64_t)( (uint64_t)anNormal[ 0 ] + (uint64_t)( dn[ 1 ] * dp[ 2 ] - dn[ 2 ] * dp[ 1 ] ) );
                    anNormal[ 1 ] = (int64_t)( (uint64_t)anNormal[ 1 ] + (uint64_t)( dn[ 2 ] * dp[ 0 ] - dn[ 0 ] * dp[ 2 ] ) );
                    anNormal[ 2 ] = (int64_t)( (uint64_t)anNormal[ 2 ] + (uint64_t)( dn[ 0 ] * dp[ 1 ] - dn[ 1 ] * dp[ 0 ] ) );
                    if ( bLeft )
                    {
                        c = t.SwingLeft( c );
                        if ( c < 0 )
                        {
                            c = t.SwingRight( nCorner );
                            bLeft = false;
                        }
                        else if ( c == nCorner )
                            c = INVALID;
                    }
                    else
                        c = t.SwingRight( c );
                }

                int64_t nAbsSum = std::llabs( anNormal[ 0 ] ) + std::llabs( anNormal[ 1 ] ) + std::llabs( anNormal[ 2 ] );
                const int64_t nUpper = (int64_t)1 << 29;

                if ( nAbsSum > nUpper )
                {
                    int64_t nQuotient = nAbsSum / nUpper;

                    for ( int k = 0; k < 3; k++ )
                        anNormal[ k ] /= nQuotient;
                }
                for ( int k = 0; k < 3; k++ )
                    anNormal[ k ] = (int32_t)anNormal[ k ];
                oOctahedron.Canonicalize( anNormal );
                if ( oFlips.Next() )
                {
                    for ( int k = 0; k < 3; k++ )
                        anNormal[ k ] = -anNormal[ k ];
                }

                int32_t anOct[ 2 ];

                oOctahedron.ToCoords( anNormal, anOct[ 0 ], anOct[ 1 ] );
                Original( anOct, &anCorr[ (size_t)nValue * 2 ], &pData[ (size_t)nValue * 2 ] );
            }
        }
    }

    void CDecoder::Attributes( void )
    {
        CReader &r = m_oReader;
        int      nDecoders = r.U8();
        struct SDecoderHead { int nDataId = -1; int nType = MESH_VERTEX_ATTRIBUTE; int nTraversal = TRAVERSAL_DEPTH_FIRST; };
        std::vector<SDecoderHead> aoHeads( (size_t)nDecoders );

        if ( m_nMethod == MESH_EDGEBREAKER )
        {
            for ( int i = 0; i < nDecoders; i++ )
            {
                aoHeads[ (size_t)i ].nDataId = r.I8();
                aoHeads[ (size_t)i ].nType = r.U8();
                aoHeads[ (size_t)i ].nTraversal = r.U8();
                if ( aoHeads[ (size_t)i ].nDataId >= (int)m_apSeams.size() )
                    throw SFail{ __LINE__ };
                if ( aoHeads[ (size_t)i ].nType == MESH_CORNER_ATTRIBUTE &&
                     ( aoHeads[ (size_t)i ].nDataId < 0 || aoHeads[ (size_t)i ].nTraversal != TRAVERSAL_DEPTH_FIRST ) )
                    throw SFail{ __LINE__ };
                if ( aoHeads[ (size_t)i ].nTraversal != TRAVERSAL_DEPTH_FIRST && aoHeads[ (size_t)i ].nTraversal != TRAVERSAL_PREDICTION_DEGREE )
                    throw SFail{ __LINE__ };
            }
        }

        std::vector<std::vector<int> > aanAttributes( (size_t)nDecoders );

        for ( int i = 0; i < nDecoders; i++ )
        {
            uint32_t nCount = r.Var32();

            if ( nCount > 256 )
                throw SFail{ __LINE__ };
            for ( uint32_t j = 0; j < nCount; j++ )
            {
                SAttributeData oData;

                oData.nType = r.U8();
                oData.nDataType = r.U8();
                oData.nComponents = r.U8();
                r.U8();     // normalized
                oData.nUniqueId = (int)r.Var32();
                oData.nDecoder = i;
                if ( oData.nType > 4 || oData.nDataType < 1 || oData.nDataType > 10 || oData.nComponents < 1 || oData.nComponents > 4 )
                    throw SFail{ __LINE__ };
                aanAttributes[ (size_t)i ].push_back( (int)m_aoAttributes.size() );
                m_aoAttributes.push_back( oData );
            }
            for ( uint32_t j = 0; j < nCount; j++ )
            {
                SAttributeData &oData = m_aoAttributes[ (size_t)aanAttributes[ (size_t)i ][ j ] ];

                oData.nDecoderType = r.U8();
                if ( oData.nDecoderType > DECODER_NORMALS )
                    throw SFail{ __LINE__ };
                if ( oData.nDecoderType == DECODER_NORMALS && oData.nComponents != 3 )
                    throw SFail{ __LINE__ };
                oData.nPortableComponents = ( oData.nDecoderType == DECODER_NORMALS ) ? 2 : oData.nComponents;
                if ( oData.nType == Draco::ATTRIBUTE_POSITION && m_nPositionAttribute < 0 )
                    m_nPositionAttribute = aanAttributes[ (size_t)i ][ j ];
            }
        }

            // Decoders run in order; each traverses its own corner table.
        for ( int i = 0; i < nDecoders; i++ )
        {
            STraversal oTraversal;
            int        nPoints = 0;

            if ( m_nMethod == MESH_EDGEBREAKER )
            {
                const SDecoderHead &oHead = aoHeads[ (size_t)i ];

                oTraversal.pTable = ( oHead.nType == MESH_CORNER_ATTRIBUTE ) ? &m_apSeams[ (size_t)oHead.nDataId ]->oTable : &m_oTable;

                const SCornerTable &t = *oTraversal.pTable;

                oTraversal.abFaceVisited.assign( (size_t)m_nFaces, 0 );
                oTraversal.abVertexVisited.assign( (size_t)t.Vertices(), 0 );
                oTraversal.anVertexValue.assign( (size_t)t.Vertices(), INVALID );
                if ( oHead.nTraversal == TRAVERSAL_DEPTH_FIRST )
                {
                    std::vector<int> anStack;

                    for ( int f = 0; f < m_nFaces; f++ )
                        oTraversal.DepthFirst( 3 * f, anStack );
                }
                else
                {
                    std::vector<int> aanStacks[ 3 ];
                    std::vector<int> anDegree( (size_t)t.Vertices(), 0 );

                    for ( int f = 0; f < m_nFaces; f++ )
                        oTraversal.PredictionDegree( 3 * f, aanStacks, anDegree );
                }
                nPoints = (int)oTraversal.anValueCorner.size();
            }
            else
            {
                    // One value a point, in order.
                oTraversal.pTable = &m_oTable;
                nPoints = m_oTable.Vertices();
                oTraversal.anVertexValue.resize( (size_t)nPoints );
                for ( int p = 0; p < nPoints; p++ )
                    oTraversal.anVertexValue[ (size_t)p ] = p;
            }

            for ( int nAttribute : aanAttributes[ (size_t)i ] )
            {
                SAttributeData &oData = m_aoAttributes[ (size_t)nAttribute ];

                if ( oData.nDecoderType == DECODER_GENERIC )
                {
                        // Raw values of the attribute's own type.
                    static const int s_anSize[ 11 ] = { 0, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8 };
                    int              nSize = s_anSize[ oData.nDataType ];

                    if ( nSize != 1 && nSize != 2 && nSize != 4 )
                        throw SFail{ __LINE__ };
                    oData.afValues.resize( (size_t)nPoints * (size_t)oData.nComponents );
                    for ( size_t k = 0; k < oData.afValues.size(); k++ )
                    {
                        uint32_t n = 0;

                        for ( int b = 0; b < nSize; b++ )
                            n |= (uint32_t)r.U8() << ( 8 * b );
                        if ( oData.nDataType == 9 )
                        {
                            float f;

                            memcpy( &f, &n, 4 );
                            oData.afValues[ k ] = f;
                        }
                        else if ( oData.nDataType == 1 ) oData.afValues[ k ] = (float)(int8_t)n;
                        else if ( oData.nDataType == 3 ) oData.afValues[ k ] = (float)(int16_t)n;
                        else if ( oData.nDataType == 5 ) oData.afValues[ k ] = (float)(int32_t)n;
                        else oData.afValues[ k ] = (float)n;
                    }
                    continue;
                }
                    // The position table's corner values are what the other
                    // attributes' predictions read positions through.
                if ( nAttribute == m_nPositionAttribute )
                {
                    m_anPositionCornerValue.assign( (size_t)m_nFaces * 3, INVALID );
                    for ( int c = 0; c < m_nFaces * 3; c++ )
                        m_anPositionCornerValue[ (size_t)c ] = oTraversal.anVertexValue[ (size_t)oTraversal.pTable->Vertex( c ) ];
                }
                DecodeValues( oData, oTraversal, nPoints );
            }
                // Then the data each value transform needs, and the values.
            for ( int nAttribute : aanAttributes[ (size_t)i ] )
            {
                SAttributeData &oData = m_aoAttributes[ (size_t)nAttribute ];

                if ( oData.nDecoderType == DECODER_NORMALS )
                {
                    int         nBits = r.U8();
                    SOctahedron oOctahedron;

                    oOctahedron.SetBits( nBits );
                    oData.afValues.resize( (size_t)nPoints * 3 );
                    for ( int p = 0; p < nPoints; p++ )
                        oOctahedron.ToVector( oData.anPortable[ (size_t)p * 2 ], oData.anPortable[ (size_t)p * 2 + 1 ],
                                              &oData.afValues[ (size_t)p * 3 ] );
                }
                else if ( oData.nDecoderType == DECODER_QUANTIZATION )
                {
                    std::vector<float> afMinimum( (size_t)oData.nComponents );

                    for ( int k = 0; k < oData.nComponents; k++ )
                        afMinimum[ (size_t)k ] = r.F32();

                    float fRange = r.F32();
                    int   nBits = r.U8();

                    if ( nBits < 1 || nBits > 30 )
                        throw SFail{ __LINE__ };

                    int32_t nMaxQuantized = ( 1 << nBits ) - 1;
                    float   fDelta = fRange / (float)nMaxQuantized;

                    oData.afValues.resize( (size_t)nPoints * (size_t)oData.nComponents );
                    for ( size_t k = 0; k < oData.afValues.size(); k++ )
                        oData.afValues[ k ] = (float)oData.anPortable[ k ] * fDelta + afMinimum[ k % (size_t)oData.nComponents ];
                }
                else if ( oData.nDecoderType == DECODER_INTEGER )
                {
                    oData.afValues.resize( oData.anPortable.size() );
                    for ( size_t k = 0; k < oData.afValues.size(); k++ )
                        oData.afValues[ k ] = (float)oData.anPortable[ k ];
                }
            }

                // Each corner's value.
            m_aoCornerValues.resize( m_aoAttributes.size() );
            for ( int nAttribute : aanAttributes[ (size_t)i ] )
            {
                std::vector<int> &anCorner = m_aoCornerValues[ (size_t)nAttribute ];

                anCorner.assign( (size_t)m_nFaces * 3, INVALID );
                for ( int c = 0; c < m_nFaces * 3; c++ )
                {
                    int v = oTraversal.pTable->Vertex( c );

                    anCorner[ (size_t)c ] = ( v >= 0 && (size_t)v < oTraversal.anVertexValue.size() ) ? oTraversal.anVertexValue[ (size_t)v ] : INVALID;
                }
            }
        }
    }
}

namespace
{
    void CDecoder::Decode( Draco::SMesh &oMesh, std::string &strError )
    {
        CReader &r = m_oReader;

        if ( r.Left() < 11 || memcmp( r.Here(), "DRACO", 5 ) != 0 )
        {
            strError = "isn't Draco data";
            throw SFail{ __LINE__ };
        }
        r.Skip( 5 );

        int nMajor = r.U8();
        int nMinor = r.U8();
        int nType = r.U8();

        m_nMethod = r.U8();

        int nFlags = r.U8();

        nFlags |= r.U8() << 8;
        if ( nMajor != 2 || nMinor != 2 )
        {
            strError = "is Draco version " + std::to_string( nMajor ) + "." + std::to_string( nMinor ) +
                       ", and Ray decodes 2.2";
            throw SFail{ __LINE__ };
        }
        if ( nType != 1 )
        {
            strError = "is a Draco point cloud, not a mesh";
            throw SFail{ __LINE__ };
        }
        if ( m_nMethod != MESH_SEQUENTIAL && m_nMethod != MESH_EDGEBREAKER )
        {
            strError = "uses an unknown Draco encoding";
            throw SFail{ __LINE__ };
        }
        if ( nFlags & 0x8000 )
        {
                // Metadata: skipped.
            std::function<void( void )> Element = [ & ]()
            {
                uint32_t nEntries = r.Var32();

                for ( uint32_t i = 0; i < nEntries; i++ )
                {
                    r.Skip( r.U8() );
                    r.Skip( r.U8() );
                }

                uint32_t nSubs = r.Var32();

                for ( uint32_t i = 0; i < nSubs; i++ )
                {
                    r.Skip( r.U8() );
                    Element();
                }
            };
            uint32_t nAttributeMetadata = r.Var32();

            for ( uint32_t i = 0; i < nAttributeMetadata; i++ )
            {
                r.Var32();
                Element();
            }
            Element();
        }
        Connectivity( strError );
        Attributes();

        oMesh.nFaces = m_nFaces;
        oMesh.aoAttributes.clear();
        for ( size_t i = 0; i < m_aoAttributes.size(); i++ )
        {
            Draco::SAttribute oOut;

            oOut.nType = m_aoAttributes[ i ].nType;
            oOut.nUniqueId = m_aoAttributes[ i ].nUniqueId;
            oOut.nComponents = m_aoAttributes[ i ].nComponents;
            oOut.afValues.swap( m_aoAttributes[ i ].afValues );
            if ( i < m_aoCornerValues.size() )
                oOut.anCornerValue.swap( m_aoCornerValues[ i ] );

            size_t nValues = oOut.afValues.size() / (size_t)oOut.nComponents;

            for ( int nValue : oOut.anCornerValue )
            {
                if ( nValue < 0 || (size_t)nValue >= nValues )
                {
                    strError = "has an attribute value missing at a corner";
                    throw SFail{ __LINE__ };
                }
            }
            oMesh.aoAttributes.push_back( oOut );
        }
    }
}

bool Draco::Decode( const unsigned char *pData, size_t nSize, SMesh &oMesh, std::string &strError )
{
    CDecoder oDecoder( pData, nSize );

    strError.clear();
    try
    {
        oDecoder.Decode( oMesh, strError );
    }
    catch ( const SFail & )
    {
        if ( strError.empty() )
            strError = "has damaged or unsupported Draco data";
        return false;
    }
    catch ( const std::bad_alloc & )
    {
        strError = "has Draco data too large to decode";
        return false;
    }
    return true;
}

const Draco::SAttribute *Draco::SMesh::ByUniqueId( int nUniqueId ) const
{
    for ( const SAttribute &oAttribute : aoAttributes )
    {
        if ( oAttribute.nUniqueId == nUniqueId )
            return &oAttribute;
    }
    return NULL;
}
