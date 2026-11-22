/*************************************************************************
*   CWebpImage                                                           *
*                                                                        *
*   A WebP decoder of our own: the RIFF container, lossless VP8L (RFC    *
*   9649), lossy VP8 key frames (RFC 6386) and alpha. See WebpImage.h.   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#include "WebpImage.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace
{
    const int64_t kMaxPixels = (int64_t)64 * 1024 * 1024;

    inline int Clip255( int v ) { return ( v < 0 ) ? 0 : ( v > 255 ) ? 255 : v; }

    // =====================================================================
    // Lossless (VP8L)
    // =====================================================================

        // LSB-first bits, as VP8L packs them.
    class CLsbBits
    {
        private:
            const BYTE *m_p;
            size_t      m_nSize;
            size_t      m_nAt;
            uint64_t    m_nBits;
            int         m_nCount;
            int         m_nOver;        // bytes fed past the end

            void Fill( void )
            {
                while ( m_nCount <= 56 )
                {
                    uint64_t nByte = 0;

                    if ( m_nAt < m_nSize )
                        nByte = m_p[ m_nAt++ ];
                    else
                        m_nOver++;
                    m_nBits |= nByte << m_nCount;
                    m_nCount += 8;
                }
            }

        public:
            CLsbBits( const BYTE *p, size_t nSize ) : m_p( p ), m_nSize( nSize ), m_nAt( 0 ), m_nBits( 0 ), m_nCount( 0 ), m_nOver( 0 ) {}

            uint32_t Read( int nBits )
            {
                if ( nBits == 0 )
                    return 0;
                Fill();

                uint32_t nValue = (uint32_t)( m_nBits & ( ( (uint64_t)1 << nBits ) - 1 ) );

                m_nBits >>= nBits;
                m_nCount -= nBits;
                return nValue;
            }

            uint32_t Peek( int nBits )
            {
                Fill();
                return (uint32_t)( m_nBits & ( ( (uint64_t)1 << nBits ) - 1 ) );
            }

            void Skip( int nBits )
            {
                m_nBits >>= nBits;
                m_nCount -= nBits;
            }

                /// Whether more was read than the data holds.
            BOOL Overrun( void ) const { return m_nOver * 8 > m_nCount + 64; }
    };

        // A canonical prefix code, read with the code's first bit first.
    class CPrefix
    {
        private:
            enum { kTableBits = 10 };

            int                     m_nSingle;              // the symbol, when there is only one
            std::vector<uint32_t>   m_anTable;              // (length << 16) | symbol, or 0
            int                     m_anCount[ 16 ];
            std::vector<int>        m_anSymbols;            // by length, then symbol

        public:
            CPrefix( void ) : m_nSingle( -1 ) { memset( m_anCount, 0, sizeof( m_anCount ) ); }

                /// FALSE if the lengths don't make a complete code.
            BOOL Build( const std::vector<int> &anLengths )
            {
                int nUsed = 0, nLast = 0;

                memset( m_anCount, 0, sizeof( m_anCount ) );
                for ( size_t n = 0; n < anLengths.size(); n++ )
                    if ( anLengths[ n ] )
                    {
                        m_anCount[ anLengths[ n ] ]++;
                        nUsed++;
                        nLast = (int)n;
                    }
                m_anSymbols.clear();
                m_anTable.clear();
                m_nSingle = -1;
                if ( nUsed == 0 )
                    return FALSE;
                if ( nUsed == 1 )
                {
                    m_nSingle = nLast;
                    return TRUE;
                }

                    // A complete code: the lengths' Kraft sum is exactly 1.
                int64_t nKraft = 0;

                for ( int nLength = 1; nLength < 16; nLength++ )
                    nKraft += (int64_t)m_anCount[ nLength ] << ( 15 - nLength );
                if ( nKraft != ( 1 << 15 ) )
                    return FALSE;

                for ( int nLength = 1; nLength < 16; nLength++ )
                    for ( size_t n = 0; n < anLengths.size(); n++ )
                        if ( anLengths[ n ] == nLength )
                            m_anSymbols.push_back( (int)n );
                m_anTable.assign( (size_t)1 << kTableBits, 0 );

                int nCode = 0, nIndex = 0;

                for ( int nLength = 1; nLength < 16; nLength++ )
                {
                    for ( int k = 0; k < m_anCount[ nLength ]; k++, nCode++, nIndex++ )
                    {
                        if ( nLength > kTableBits )
                            continue;

                        int nReversed = 0;

                        for ( int b = 0; b < nLength; b++ )
                            if ( nCode & ( 1 << b ) )
                                nReversed |= 1 << ( nLength - 1 - b );
                        for ( int nFill = nReversed; nFill < ( 1 << kTableBits ); nFill += 1 << nLength )
                            m_anTable[ (size_t)nFill ] = ( (uint32_t)nLength << 16 ) | (uint32_t)m_anSymbols[ (size_t)nIndex ];
                    }
                    nCode <<= 1;
                }
                return TRUE;
            }

            int Decode( CLsbBits &oBits ) const
            {
                if ( m_nSingle >= 0 )
                    return m_nSingle;

                uint32_t nEntry = m_anTable[ oBits.Peek( kTableBits ) ];

                if ( nEntry )
                {
                    oBits.Skip( (int)( nEntry >> 16 ) );
                    return (int)( nEntry & 0xFFFF );
                }

                    // Longer than the table: bit by bit, as zlib's puff does.
                int nCode = 0, nFirst = 0, nIndex = 0;

                for ( int nLength = 1; nLength < 16; nLength++ )
                {
                    nCode |= (int)oBits.Read( 1 );

                    int nCount = m_anCount[ nLength ];

                    if ( nCode - nCount < nFirst )
                        return m_anSymbols[ (size_t)( nIndex + ( nCode - nFirst ) ) ];
                    nIndex += nCount;
                    nFirst += nCount;
                    nFirst <<= 1;
                    nCode <<= 1;
                }
                return 0;
            }
    };

    inline uint32_t AddPixels( uint32_t a, uint32_t b )
    {
        return ( ( ( a & 0xFF00FF00u ) + ( b & 0xFF00FF00u ) ) & 0xFF00FF00u ) | ( ( ( a & 0x00FF00FFu ) + ( b & 0x00FF00FFu ) ) & 0x00FF00FFu );
    }

    inline uint32_t Average2( uint32_t a, uint32_t b ) { return ( ( ( a ^ b ) & 0xFEFEFEFEu ) >> 1 ) + ( a & b ); }

    inline int Channel( uint32_t p, int nShift ) { return (int)( ( p >> nShift ) & 0xFF ); }

    uint32_t Select( uint32_t L, uint32_t T, uint32_t TL )
    {
        int pL = 0, pT = 0;

        for ( int nShift = 0; nShift < 32; nShift += 8 )
        {
            int nEstimate = Channel( L, nShift ) + Channel( T, nShift ) - Channel( TL, nShift );

            pL += abs( nEstimate - Channel( L, nShift ) );
            pT += abs( nEstimate - Channel( T, nShift ) );
        }
        return ( pL < pT ) ? L : T;
    }

    uint32_t ClampAddSubtractFull( uint32_t a, uint32_t b, uint32_t c )
    {
        uint32_t nOut = 0;

        for ( int nShift = 0; nShift < 32; nShift += 8 )
            nOut |= (uint32_t)Clip255( Channel( a, nShift ) + Channel( b, nShift ) - Channel( c, nShift ) ) << nShift;
        return nOut;
    }

    uint32_t ClampAddSubtractHalf( uint32_t a, uint32_t b )
    {
        uint32_t nOut = 0;

        for ( int nShift = 0; nShift < 32; nShift += 8 )
        {
            int x = Channel( a, nShift ), y = Channel( b, nShift );

            nOut |= (uint32_t)Clip255( x + ( x - y ) / 2 ) << nShift;
        }
        return nOut;
    }

    inline int ColorTransformDelta( int8_t t, int8_t c ) { return ( (int)t * (int)c ) >> 5; }

        // RFC 9649's offsets for the 120 short distance codes: (x, y).
    const signed char kDistanceMap[ 120 ][ 2 ] =
    {
        {0, 1},  {1, 0},  {1, 1},  {-1, 1}, {0, 2},  {2, 0},  {1, 2},
        {-1, 2}, {2, 1},  {-2, 1}, {2, 2},  {-2, 2}, {0, 3},  {3, 0},
        {1, 3},  {-1, 3}, {3, 1},  {-3, 1}, {2, 3},  {-2, 3}, {3, 2},
        {-3, 2}, {0, 4},  {4, 0},  {1, 4},  {-1, 4}, {4, 1},  {-4, 1},
        {3, 3},  {-3, 3}, {2, 4},  {-2, 4}, {4, 2},  {-4, 2}, {0, 5},
        {3, 4},  {-3, 4}, {4, 3},  {-4, 3}, {5, 0},  {1, 5},  {-1, 5},
        {5, 1},  {-5, 1}, {2, 5},  {-2, 5}, {5, 2},  {-5, 2}, {4, 4},
        {-4, 4}, {3, 5},  {-3, 5}, {5, 3},  {-5, 3}, {0, 6},  {6, 0},
        {1, 6},  {-1, 6}, {6, 1},  {-6, 1}, {2, 6},  {-2, 6}, {6, 2},
        {-6, 2}, {4, 5},  {-4, 5}, {5, 4},  {-5, 4}, {3, 6},  {-3, 6},
        {6, 3},  {-6, 3}, {0, 7},  {7, 0},  {1, 7},  {-1, 7}, {5, 5},
        {-5, 5}, {7, 1},  {-7, 1}, {4, 6},  {-4, 6}, {6, 4},  {-6, 4},
        {2, 7},  {-2, 7}, {7, 2},  {-7, 2}, {3, 7},  {-3, 7}, {7, 3},
        {-7, 3}, {5, 6},  {-5, 6}, {6, 5},  {-6, 5}, {8, 0},  {4, 7},
        {-4, 7}, {7, 4},  {-7, 4}, {8, 1},  {8, 2},  {6, 6},  {-6, 6},
        {8, 3},  {5, 7},  {-5, 7}, {7, 5},  {-7, 5}, {8, 4},  {6, 7},
        {-6, 7}, {7, 6},  {-7, 6}, {8, 5},  {7, 7},  {-7, 7}, {8, 6},
        {8, 7}
    };

    class CLossless
    {
        private:
            struct STransform
            {
                int                     nType;
                int                     nWidth;     // the image's width when it was read
                int                     nBits;      // block size bits, or color-indexing width bits
                std::vector<uint32_t>   anData;     // the sub-image or the color table
            };

            CLsbBits        m_oBits;
            std::string     m_strError;

            BOOL Fail( const std::string &strWhy )
            {
                if ( m_strError.empty() )
                    m_strError = strWhy;
                return FALSE;
            }

            static int DivRoundUp( int n, int nBits ) { return ( n + ( 1 << nBits ) - 1 ) >> nBits; }

            BOOL ReadCode( int nAlphabet, CPrefix &oCode );
            int  Prefixed( int nSymbol );
            BOOL DecodeImage( int nWidth, int nHeight, BOOL bArgb, std::vector<uint32_t> &anOut );
            void Inverse( const STransform &oTransform, int nHeight, std::vector<uint32_t> &anImage );

        public:
            CLossless( const BYTE *p, size_t nSize ) : m_oBits( p, nSize ) {}

            const std::string &Error( void ) const { return m_strError; }

                /// The image data after the header: transforms, then the
                /// image (as the VP8L chunk holds it after its 5 bytes, and
                /// the ALPH chunk holds it whole).
            BOOL Decode( int nWidth, int nHeight, std::vector<uint32_t> &anArgb );
                /// A whole VP8L chunk, with its signature and size.
            BOOL DecodeChunk( int &nWidth, int &nHeight, std::vector<uint32_t> &anArgb );
    };

    BOOL CLossless::ReadCode( int nAlphabet, CPrefix &oCode )
    {
        std::vector<int> anLengths( (size_t)nAlphabet, 0 );

        if ( m_oBits.Read( 1 ) )
        {
                // A simple code of one or two symbols.
            int nSymbols = (int)m_oBits.Read( 1 ) + 1;
            int nFirst = (int)m_oBits.Read( m_oBits.Read( 1 ) ? 8 : 1 );

            if ( nFirst >= nAlphabet )
                return Fail( "has a damaged prefix code" );
            anLengths[ (size_t)nFirst ] = 1;
            if ( nSymbols == 2 )
            {
                int nSecond = (int)m_oBits.Read( 8 );

                if ( nSecond >= nAlphabet )
                    return Fail( "has a damaged prefix code" );
                anLengths[ (size_t)nSecond ] = 1;
            }
        }
        else
        {
            static const int kOrder[ 19 ] = { 17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
            std::vector<int> anCodeLengths( 19, 0 );
            int              nCount = 4 + (int)m_oBits.Read( 4 );
            CPrefix          oLengthCode;

            for ( int n = 0; n < nCount; n++ )
                anCodeLengths[ (size_t)kOrder[ n ] ] = (int)m_oBits.Read( 3 );
            if ( ! oLengthCode.Build( anCodeLengths ) )
                return Fail( "has a damaged prefix code" );

            int nMax = nAlphabet;

            if ( m_oBits.Read( 1 ) )
            {
                int nBits = 2 + 2 * (int)m_oBits.Read( 3 );

                nMax = 2 + (int)m_oBits.Read( nBits );
                if ( nMax > nAlphabet )
                    return Fail( "has a damaged prefix code" );
            }

            int nPrevious = 8;

            for ( int nSymbol = 0; nSymbol < nAlphabet; )
            {
                if ( nMax-- == 0 )
                    break;

                int nLength = oLengthCode.Decode( m_oBits );

                if ( nLength < 16 )
                {
                    anLengths[ (size_t)nSymbol++ ] = nLength;
                    if ( nLength )
                        nPrevious = nLength;
                    continue;
                }

                int nRepeat, nValue;

                if ( nLength == 16 )
                {
                    nRepeat = 3 + (int)m_oBits.Read( 2 );
                    nValue = nPrevious;
                }
                else if ( nLength == 17 )
                {
                    nRepeat = 3 + (int)m_oBits.Read( 3 );
                    nValue = 0;
                }
                else
                {
                    nRepeat = 11 + (int)m_oBits.Read( 7 );
                    nValue = 0;
                }
                if ( nSymbol + nRepeat > nAlphabet )
                    return Fail( "has a damaged prefix code" );
                while ( nRepeat-- > 0 )
                    anLengths[ (size_t)nSymbol++ ] = nValue;
            }
        }
        if ( m_oBits.Overrun() )
            return Fail( "is cut off" );
        if ( ! oCode.Build( anLengths ) )
            return Fail( "has a damaged prefix code" );
        return TRUE;
    }

        // A length or distance from its prefix symbol and extra bits.
    int CLossless::Prefixed( int nSymbol )
    {
        if ( nSymbol < 4 )
            return nSymbol + 1;

        int nExtra = ( nSymbol - 2 ) >> 1;
        int nOffset = ( 2 + ( nSymbol & 1 ) ) << nExtra;

        return nOffset + (int)m_oBits.Read( nExtra ) + 1;
    }

        // An entropy-coded image: the ARGB image itself (bArgb, which may
        // use meta prefix codes) or one of its sub-images.
    BOOL CLossless::DecodeImage( int nWidth, int nHeight, BOOL bArgb, std::vector<uint32_t> &anOut )
    {
        int nCacheBits = 0;

        if ( m_oBits.Read( 1 ) )
        {
            nCacheBits = (int)m_oBits.Read( 4 );
            if ( nCacheBits < 1 || nCacheBits > 11 )
                return Fail( "has a damaged color cache" );
        }

        int                   nMetaBits = 0, nMetaWidth = 0, nGroups = 1;
        std::vector<uint32_t> anMeta;

        if ( bArgb && m_oBits.Read( 1 ) )
        {
            nMetaBits = (int)m_oBits.Read( 3 ) + 2;
            nMetaWidth = DivRoundUp( nWidth, nMetaBits );
            if ( ! DecodeImage( nMetaWidth, DivRoundUp( nHeight, nMetaBits ), FALSE, anMeta ) )
                return FALSE;
            for ( size_t n = 0; n < anMeta.size(); n++ )
            {
                int nGroup = (int)( ( anMeta[ n ] >> 8 ) & 0xFFFF );

                if ( nGroup + 1 > nGroups )
                    nGroups = nGroup + 1;
            }
        }

        int                  nCacheSize = nCacheBits ? ( 1 << nCacheBits ) : 0;
        std::vector<CPrefix> aoCodes( (size_t)nGroups * 5 );
        static const int     kAlphabets[ 5 ] = { 256 + 24, 256, 256, 256, 40 };

        for ( int nGroup = 0; nGroup < nGroups; nGroup++ )
            for ( int nCode = 0; nCode < 5; nCode++ )
                if ( ! ReadCode( kAlphabets[ nCode ] + ( nCode == 0 ? nCacheSize : 0 ), aoCodes[ (size_t)nGroup * 5 + nCode ] ) )
                    return FALSE;

        std::vector<uint32_t> anCache( (size_t)nCacheSize, 0 );
        size_t                nTotal = (size_t)nWidth * (size_t)nHeight;
        size_t                nAt = 0;
        int                   x = 0, y = 0;

        anOut.assign( nTotal, 0 );

        auto Remember = [&]( uint32_t nArgb )
        {
            if ( nCacheBits )
                anCache[ ( 0x1E35A7BDu * nArgb ) >> ( 32 - nCacheBits ) ] = nArgb;
        };

        while ( nAt < nTotal )
        {
            const CPrefix *pCodes = &aoCodes[ 0 ];

            if ( nMetaBits )
                pCodes = &aoCodes[ (size_t)( ( anMeta[ (size_t)( y >> nMetaBits ) * (size_t)nMetaWidth + (size_t)( x >> nMetaBits ) ] >> 8 ) & 0xFFFF ) * 5 ];

            int nSymbol = pCodes[ 0 ].Decode( m_oBits );

            if ( nSymbol < 256 )
            {
                uint32_t nRed = (uint32_t)pCodes[ 1 ].Decode( m_oBits );
                uint32_t nBlue = (uint32_t)pCodes[ 2 ].Decode( m_oBits );
                uint32_t nAlpha = (uint32_t)pCodes[ 3 ].Decode( m_oBits );
                uint32_t nArgb = ( nAlpha << 24 ) | ( nRed << 16 ) | ( (uint32_t)nSymbol << 8 ) | nBlue;

                anOut[ nAt++ ] = nArgb;
                Remember( nArgb );
                if ( ++x == nWidth )
                {
                    x = 0;
                    y++;
                }
            }
            else if ( nSymbol < 256 + 24 )
            {
                int nLength = Prefixed( nSymbol - 256 );
                int nDistanceSymbol = pCodes[ 4 ].Decode( m_oBits );
                int nDistance = Prefixed( nDistanceSymbol );

                if ( nDistance > 120 )
                    nDistance -= 120;
                else
                {
                    nDistance = kDistanceMap[ nDistance - 1 ][ 0 ] + kDistanceMap[ nDistance - 1 ][ 1 ] * nWidth;
                    if ( nDistance < 1 )
                        nDistance = 1;
                }
                if ( (size_t)nDistance > nAt || nAt + (size_t)nLength > nTotal )
                    return Fail( "has damaged image data" );
                for ( int n = 0; n < nLength; n++ )
                {
                    uint32_t nArgb = anOut[ nAt - (size_t)nDistance ];

                    anOut[ nAt++ ] = nArgb;
                    Remember( nArgb );
                }
                x += nLength;
                while ( x >= nWidth )
                {
                    x -= nWidth;
                    y++;
                }
            }
            else
            {
                int nIndex = nSymbol - 256 - 24;

                if ( nIndex >= nCacheSize )
                    return Fail( "has damaged image data" );

                uint32_t nArgb = anCache[ (size_t)nIndex ];

                anOut[ nAt++ ] = nArgb;
                Remember( nArgb );
                if ( ++x == nWidth )
                {
                    x = 0;
                    y++;
                }
            }
            if ( m_oBits.Overrun() )
                return Fail( "is cut off" );
        }
        return TRUE;
    }

    void CLossless::Inverse( const STransform &oTransform, int nHeight, std::vector<uint32_t> &anImage )
    {
        int w = oTransform.nWidth;

        switch ( oTransform.nType )
        {
            case 0:     // predictor
            {
                int nBlockWidth = DivRoundUp( w, oTransform.nBits );

                for ( int y = 0; y < nHeight; y++ )
                    for ( int x = 0; x < w; x++ )
                    {
                        size_t   nAt = (size_t)y * w + x;
                        uint32_t nPredicted;

                        if ( x == 0 && y == 0 )
                            nPredicted = 0xFF000000u;
                        else if ( y == 0 )
                            nPredicted = anImage[ nAt - 1 ];
                        else if ( x == 0 )
                            nPredicted = anImage[ nAt - w ];
                        else
                        {
                            uint32_t L = anImage[ nAt - 1 ], T = anImage[ nAt - w ], TL = anImage[ nAt - w - 1 ];
                            uint32_t TR = anImage[ nAt - w + 1 ];     // the row's first pixel at the right edge
                            int      nMode = (int)( ( oTransform.anData[ (size_t)( y >> oTransform.nBits ) * nBlockWidth + ( x >> oTransform.nBits ) ] >> 8 ) & 15 );

                            switch ( nMode )
                            {
                                case 0: nPredicted = 0xFF000000u; break;
                                case 1: nPredicted = L; break;
                                case 2: nPredicted = T; break;
                                case 3: nPredicted = TR; break;
                                case 4: nPredicted = TL; break;
                                case 5: nPredicted = Average2( Average2( L, TR ), T ); break;
                                case 6: nPredicted = Average2( L, TL ); break;
                                case 7: nPredicted = Average2( L, T ); break;
                                case 8: nPredicted = Average2( TL, T ); break;
                                case 9: nPredicted = Average2( T, TR ); break;
                                case 10: nPredicted = Average2( Average2( L, TL ), Average2( T, TR ) ); break;
                                case 11: nPredicted = Select( L, T, TL ); break;
                                case 12: nPredicted = ClampAddSubtractFull( L, T, TL ); break;
                                case 13: nPredicted = ClampAddSubtractHalf( Average2( L, T ), TL ); break;
                                default: nPredicted = 0xFF000000u; break;     // 14 and 15 aren't used
                            }
                        }
                        anImage[ nAt ] = AddPixels( anImage[ nAt ], nPredicted );
                    }
                break;
            }
            case 1:     // color
            {
                int nBlockWidth = DivRoundUp( w, oTransform.nBits );

                for ( int y = 0; y < nHeight; y++ )
                    for ( int x = 0; x < w; x++ )
                    {
                        uint32_t  nCode = oTransform.anData[ (size_t)( y >> oTransform.nBits ) * nBlockWidth + ( x >> oTransform.nBits ) ];
                        uint32_t &nArgb = anImage[ (size_t)y * w + x ];
                        int8_t    nGreen = (int8_t)( nArgb >> 8 );
                        int       nRed = (int)( ( nArgb >> 16 ) & 0xFF ), nBlue = (int)( nArgb & 0xFF );

                        nRed = ( nRed + ColorTransformDelta( (int8_t)( nCode & 0xFF ), nGreen ) ) & 0xFF;
                        nBlue += ColorTransformDelta( (int8_t)( ( nCode >> 8 ) & 0xFF ), nGreen );
                        nBlue += ColorTransformDelta( (int8_t)( ( nCode >> 16 ) & 0xFF ), (int8_t)nRed );
                        nBlue &= 0xFF;
                        nArgb = ( nArgb & 0xFF00FF00u ) | ( (uint32_t)nRed << 16 ) | (uint32_t)nBlue;
                    }
                break;
            }
            case 2:     // subtract green
                for ( size_t n = 0; n < anImage.size(); n++ )
                {
                    uint32_t nArgb = anImage[ n ];
                    uint32_t nGreen = ( nArgb >> 8 ) & 0xFF;
                    uint32_t nRed = ( ( ( nArgb >> 16 ) & 0xFF ) + nGreen ) & 0xFF;
                    uint32_t nBlue = ( ( nArgb & 0xFF ) + nGreen ) & 0xFF;

                    anImage[ n ] = ( nArgb & 0xFF00FF00u ) | ( nRed << 16 ) | nBlue;
                }
                break;
            case 3:     // color indexing
            {
                int                   nBits = oTransform.nBits;
                int                   nPacked = DivRoundUp( w, nBits );
                int                   nPerByte = 1 << nBits, nWidthBits = 8 >> nBits;
                std::vector<uint32_t> anOut( (size_t)w * (size_t)nHeight );

                for ( int y = 0; y < nHeight; y++ )
                    for ( int x = 0; x < w; x++ )
                    {
                        uint32_t nGreen = ( anImage[ (size_t)y * nPacked + ( x >> nBits ) ] >> 8 ) & 0xFF;
                        uint32_t nIndex = nBits ? ( ( nGreen >> ( ( x & ( nPerByte - 1 ) ) * nWidthBits ) ) & ( ( 1u << nWidthBits ) - 1 ) ) : nGreen;

                        anOut[ (size_t)y * w + x ] = ( nIndex < oTransform.anData.size() ) ? oTransform.anData[ nIndex ] : 0;
                    }
                anImage.swap( anOut );
                break;
            }
        }
    }

    BOOL CLossless::Decode( int nWidth, int nHeight, std::vector<uint32_t> &anArgb )
    {
        std::vector<STransform> aoTransforms;
        int                     nCurrent = nWidth;
        BOOL                    abUsed[ 4 ] = { FALSE, FALSE, FALSE, FALSE };

        while ( m_oBits.Read( 1 ) )
        {
            STransform oTransform;

            oTransform.nType = (int)m_oBits.Read( 2 );
            oTransform.nWidth = nCurrent;
            oTransform.nBits = 0;
            if ( abUsed[ oTransform.nType ] )
                return Fail( "uses a transform twice" );
            abUsed[ oTransform.nType ] = TRUE;
            if ( oTransform.nType == 0 || oTransform.nType == 1 )
            {
                oTransform.nBits = (int)m_oBits.Read( 3 ) + 2;
                if ( ! DecodeImage( DivRoundUp( nCurrent, oTransform.nBits ), DivRoundUp( nHeight, oTransform.nBits ), FALSE, oTransform.anData ) )
                    return FALSE;
            }
            else if ( oTransform.nType == 3 )
            {
                int nSize = (int)m_oBits.Read( 8 ) + 1;

                if ( ! DecodeImage( nSize, 1, FALSE, oTransform.anData ) )
                    return FALSE;
                for ( size_t n = 1; n < oTransform.anData.size(); n++ )
                    oTransform.anData[ n ] = AddPixels( oTransform.anData[ n ], oTransform.anData[ n - 1 ] );
                oTransform.nBits = ( nSize <= 2 ) ? 3 : ( nSize <= 4 ) ? 2 : ( nSize <= 16 ) ? 1 : 0;
                nCurrent = DivRoundUp( nCurrent, oTransform.nBits );
            }
            aoTransforms.push_back( oTransform );
        }
        if ( ! DecodeImage( nCurrent, nHeight, TRUE, anArgb ) )
            return FALSE;
        for ( size_t n = aoTransforms.size(); n-- > 0; )
            Inverse( aoTransforms[ n ], nHeight, anArgb );
        return TRUE;
    }

    BOOL CLossless::DecodeChunk( int &nWidth, int &nHeight, std::vector<uint32_t> &anArgb )
    {
        if ( m_oBits.Read( 8 ) != 0x2F )
            return Fail( "has a damaged lossless header" );
        nWidth = (int)m_oBits.Read( 14 ) + 1;
        nHeight = (int)m_oBits.Read( 14 ) + 1;
        m_oBits.Read( 1 );      // whether alpha is used: a hint only
        if ( m_oBits.Read( 3 ) != 0 )
            return Fail( "has a lossless version Ray doesn't read" );
        if ( (int64_t)nWidth * nHeight > kMaxPixels )
            return Fail( "is too large (Ray reads WebPs of up to 64 million pixels)" );
        return Decode( nWidth, nHeight, anArgb );
    }

    // =====================================================================
    // Lossy (VP8 key frames)
    // =====================================================================

        // RFC 6386: the token probabilities' update probabilities and
        // defaults, and the key-frame subblock mode probabilities.
    static const BYTE kCoeffUpdate[ 4 ][ 8 ][ 3 ][ 11 ] =
    {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        176, 246, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        223, 241, 252, 255, 255, 255, 255, 255, 255, 255, 255,
        249, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 244, 252, 255, 255, 255, 255, 255, 255, 255, 255,
        234, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 246, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        239, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        254, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 248, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        251, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        251, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        254, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 254, 253, 255, 254, 255, 255, 255, 255, 255, 255,
        250, 255, 254, 255, 254, 255, 255, 255, 255, 255, 255,
        254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        217, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        225, 252, 241, 253, 255, 255, 254, 255, 255, 255, 255,
        234, 250, 241, 250, 253, 255, 253, 254, 255, 255, 255,
        255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        223, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        238, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255,
        255, 248, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        249, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        247, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 254, 253, 255, 255, 255, 255, 255, 255, 255, 255,
        250, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        186, 251, 250, 255, 255, 255, 255, 255, 255, 255, 255,
        234, 251, 244, 254, 255, 255, 255, 255, 255, 255, 255,
        251, 251, 243, 253, 254, 255, 254, 255, 255, 255, 255,
        255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        236, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        251, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255,
        255, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        250, 254, 252, 254, 255, 255, 255, 255, 255, 255, 255,
        248, 254, 249, 253, 255, 255, 255, 255, 255, 255, 255,
        255, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255,
        246, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255,
        252, 254, 251, 254, 254, 255, 255, 255, 255, 255, 255,
        255, 254, 252, 255, 255, 255, 255, 255, 255, 255, 255,
        248, 254, 253, 255, 255, 255, 255, 255, 255, 255, 255,
        253, 255, 254, 254, 255, 255, 255, 255, 255, 255, 255,
        255, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        245, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        253, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 251, 253, 255, 255, 255, 255, 255, 255, 255, 255,
        252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        249, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 253, 255, 255, 255, 255, 255, 255, 255, 255,
        250, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    };

    static const BYTE kDefaultCoeff[ 4 ][ 8 ][ 3 ][ 11 ] =
    {
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        253, 136, 254, 255, 228, 219, 128, 128, 128, 128, 128,
        189, 129, 242, 255, 227, 213, 255, 219, 128, 128, 128,
        106, 126, 227, 252, 214, 209, 255, 255, 128, 128, 128,
          1,  98, 248, 255, 236, 226, 255, 255, 128, 128, 128,
        181, 133, 238, 254, 221, 234, 255, 154, 128, 128, 128,
         78, 134, 202, 247, 198, 180, 255, 219, 128, 128, 128,
          1, 185, 249, 255, 243, 255, 128, 128, 128, 128, 128,
        184, 150, 247, 255, 236, 224, 128, 128, 128, 128, 128,
         77, 110, 216, 255, 236, 230, 128, 128, 128, 128, 128,
          1, 101, 251, 255, 241, 255, 128, 128, 128, 128, 128,
        170, 139, 241, 252, 236, 209, 255, 255, 128, 128, 128,
         37, 116, 196, 243, 228, 255, 255, 255, 128, 128, 128,
          1, 204, 254, 255, 245, 255, 128, 128, 128, 128, 128,
        207, 160, 250, 255, 238, 128, 128, 128, 128, 128, 128,
        102, 103, 231, 255, 211, 171, 128, 128, 128, 128, 128,
          1, 152, 252, 255, 240, 255, 128, 128, 128, 128, 128,
        177, 135, 243, 255, 234, 225, 128, 128, 128, 128, 128,
         80, 129, 211, 255, 194, 224, 128, 128, 128, 128, 128,
          1,   1, 255, 128, 128, 128, 128, 128, 128, 128, 128,
        246,   1, 255, 128, 128, 128, 128, 128, 128, 128, 128,
        255, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        198,  35, 237, 223, 193, 187, 162, 160, 145, 155,  62,
        131,  45, 198, 221, 172, 176, 220, 157, 252, 221,   1,
         68,  47, 146, 208, 149, 167, 221, 162, 255, 223, 128,
          1, 149, 241, 255, 221, 224, 255, 255, 128, 128, 128,
        184, 141, 234, 253, 222, 220, 255, 199, 128, 128, 128,
         81,  99, 181, 242, 176, 190, 249, 202, 255, 255, 128,
          1, 129, 232, 253, 214, 197, 242, 196, 255, 255, 128,
         99, 121, 210, 250, 201, 198, 255, 202, 128, 128, 128,
         23,  91, 163, 242, 170, 187, 247, 210, 255, 255, 128,
          1, 200, 246, 255, 234, 255, 128, 128, 128, 128, 128,
        109, 178, 241, 255, 231, 245, 255, 255, 128, 128, 128,
         44, 130, 201, 253, 205, 192, 255, 255, 128, 128, 128,
          1, 132, 239, 251, 219, 209, 255, 165, 128, 128, 128,
         94, 136, 225, 251, 218, 190, 255, 255, 128, 128, 128,
         22, 100, 174, 245, 186, 161, 255, 199, 128, 128, 128,
          1, 182, 249, 255, 232, 235, 128, 128, 128, 128, 128,
        124, 143, 241, 255, 227, 234, 128, 128, 128, 128, 128,
         35,  77, 181, 251, 193, 211, 255, 205, 128, 128, 128,
          1, 157, 247, 255, 236, 231, 255, 255, 128, 128, 128,
        121, 141, 235, 255, 225, 227, 255, 255, 128, 128, 128,
         45,  99, 188, 251, 195, 217, 255, 224, 128, 128, 128,
          1,   1, 251, 255, 213, 255, 128, 128, 128, 128, 128,
        203,   1, 248, 255, 255, 128, 128, 128, 128, 128, 128,
        137,   1, 177, 255, 224, 255, 128, 128, 128, 128, 128,
        253,   9, 248, 251, 207, 208, 255, 192, 128, 128, 128,
        175,  13, 224, 243, 193, 185, 249, 198, 255, 255, 128,
         73,  17, 171, 221, 161, 179, 236, 167, 255, 234, 128,
          1,  95, 247, 253, 212, 183, 255, 255, 128, 128, 128,
        239,  90, 244, 250, 211, 209, 255, 255, 128, 128, 128,
        155,  77, 195, 248, 188, 195, 255, 255, 128, 128, 128,
          1,  24, 239, 251, 218, 219, 255, 205, 128, 128, 128,
        201,  51, 219, 255, 196, 186, 128, 128, 128, 128, 128,
         69,  46, 190, 239, 201, 218, 255, 228, 128, 128, 128,
          1, 191, 251, 255, 255, 128, 128, 128, 128, 128, 128,
        223, 165, 249, 255, 213, 255, 128, 128, 128, 128, 128,
        141, 124, 248, 255, 255, 128, 128, 128, 128, 128, 128,
          1,  16, 248, 255, 255, 128, 128, 128, 128, 128, 128,
        190,  36, 230, 255, 236, 255, 128, 128, 128, 128, 128,
        149,   1, 255, 128, 128, 128, 128, 128, 128, 128, 128,
          1, 226, 255, 128, 128, 128, 128, 128, 128, 128, 128,
        247, 192, 255, 128, 128, 128, 128, 128, 128, 128, 128,
        240, 128, 255, 128, 128, 128, 128, 128, 128, 128, 128,
          1, 134, 252, 255, 255, 128, 128, 128, 128, 128, 128,
        213,  62, 250, 255, 255, 128, 128, 128, 128, 128, 128,
         55,  93, 255, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128,
        202,  24, 213, 235, 186, 191, 220, 160, 240, 175, 255,
        126,  38, 182, 232, 169, 184, 228, 174, 255, 187, 128,
         61,  46, 138, 219, 151, 178, 240, 170, 255, 216, 128,
          1, 112, 230, 250, 199, 191, 247, 159, 255, 255, 128,
        166, 109, 228, 252, 211, 215, 255, 174, 128, 128, 128,
         39,  77, 162, 232, 172, 180, 245, 178, 255, 255, 128,
          1,  52, 220, 246, 198, 199, 249, 220, 255, 255, 128,
        124,  74, 191, 243, 183, 193, 250, 221, 255, 255, 128,
         24,  71, 130, 219, 154, 170, 243, 182, 255, 255, 128,
          1, 182, 225, 249, 219, 240, 255, 224, 128, 128, 128,
        149, 150, 226, 252, 216, 205, 255, 171, 128, 128, 128,
         28, 108, 170, 242, 183, 194, 254, 223, 255, 255, 128,
          1,  81, 230, 252, 204, 203, 255, 192, 128, 128, 128,
        123, 102, 209, 247, 188, 196, 255, 233, 128, 128, 128,
         20,  95, 153, 243, 164, 173, 255, 203, 128, 128, 128,
          1, 222, 248, 255, 216, 213, 128, 128, 128, 128, 128,
        168, 175, 246, 252, 235, 205, 255, 255, 128, 128, 128,
         47, 116, 215, 255, 211, 212, 255, 255, 128, 128, 128,
          1, 121, 236, 253, 212, 214, 255, 255, 128, 128, 128,
        141,  84, 213, 252, 201, 202, 255, 219, 128, 128, 128,
         42,  80, 160, 240, 162, 185, 255, 205, 128, 128, 128,
          1,   1, 255, 128, 128, 128, 128, 128, 128, 128, 128,
        244,   1, 255, 128, 128, 128, 128, 128, 128, 128, 128,
        238,   1, 255, 128, 128, 128, 128, 128, 128, 128, 128,
    };

    static const BYTE kKeyBModeProbs[ 10 ][ 10 ][ 9 ] =
    {
        231, 120,  48,  89, 115, 113, 120, 152, 112,
        152, 179,  64, 126, 170, 118,  46,  70,  95,
        175,  69, 143,  80,  85,  82,  72, 155, 103,
         56,  58,  10, 171, 218, 189,  17,  13, 152,
        144,  71,  10,  38, 171, 213, 144,  34,  26,
        114,  26,  17, 163,  44, 195,  21,  10, 173,
        121,  24,  80, 195,  26,  62,  44,  64,  85,
        170,  46,  55,  19, 136, 160,  33, 206,  71,
         63,  20,   8, 114, 114, 208,  12,   9, 226,
         81,  40,  11,  96, 182,  84,  29,  16,  36,
        134, 183,  89, 137,  98, 101, 106, 165, 148,
         72, 187, 100, 130, 157, 111,  32,  75,  80,
         66, 102, 167,  99,  74,  62,  40, 234, 128,
         41,  53,   9, 178, 241, 141,  26,   8, 107,
        104,  79,  12,  27, 217, 255,  87,  17,   7,
         74,  43,  26, 146,  73, 166,  49,  23, 157,
         65,  38, 105, 160,  51,  52,  31, 115, 128,
         87,  68,  71,  44, 114,  51,  15, 186,  23,
         47,  41,  14, 110, 182, 183,  21,  17, 194,
         66,  45,  25, 102, 197, 189,  23,  18,  22,
         88,  88, 147, 150,  42,  46,  45, 196, 205,
         43,  97, 183, 117,  85,  38,  35, 179,  61,
         39,  53, 200,  87,  26,  21,  43, 232, 171,
         56,  34,  51, 104, 114, 102,  29,  93,  77,
        107,  54,  32,  26,  51,   1,  81,  43,  31,
         39,  28,  85, 171,  58, 165,  90,  98,  64,
         34,  22, 116, 206,  23,  34,  43, 166,  73,
         68,  25, 106,  22,  64, 171,  36, 225, 114,
         34,  19,  21, 102, 132, 188,  16,  76, 124,
         62,  18,  78,  95,  85,  57,  50,  48,  51,
        193, 101,  35, 159, 215, 111,  89,  46, 111,
         60, 148,  31, 172, 219, 228,  21,  18, 111,
        112, 113,  77,  85, 179, 255,  38, 120, 114,
         40,  42,   1, 196, 245, 209,  10,  25, 109,
        100,  80,   8,  43, 154,   1,  51,  26,  71,
         88,  43,  29, 140, 166, 213,  37,  43, 154,
         61,  63,  30, 155,  67,  45,  68,   1, 209,
        142,  78,  78,  16, 255, 128,  34, 197, 171,
         41,  40,   5, 102, 211, 183,   4,   1, 221,
         51,  50,  17, 168, 209, 192,  23,  25,  82,
        125,  98,  42,  88, 104,  85, 117, 175,  82,
         95,  84,  53,  89, 128, 100, 113, 101,  45,
         75,  79, 123,  47,  51, 128,  81, 171,   1,
         57,  17,   5,  71, 102,  57,  53,  41,  49,
        115,  21,   2,  10, 102, 255, 166,  23,   6,
         38,  33,  13, 121,  57,  73,  26,   1,  85,
         41,  10,  67, 138,  77, 110,  90,  47, 114,
        101,  29,  16,  10,  85, 128, 101, 196,  26,
         57,  18,  10, 102, 102, 213,  34,  20,  43,
        117,  20,  15,  36, 163, 128,  68,   1,  26,
        138,  31,  36, 171,  27, 166,  38,  44, 229,
         67,  87,  58, 169,  82, 115,  26,  59, 179,
         63,  59,  90, 180,  59, 166,  93,  73, 154,
         40,  40,  21, 116, 143, 209,  34,  39, 175,
         57,  46,  22,  24, 128,   1,  54,  17,  37,
         47,  15,  16, 183,  34, 223,  49,  45, 183,
         46,  17,  33, 183,   6,  98,  15,  32, 183,
         65,  32,  73, 115,  28, 128,  23, 128, 205,
         40,   3,   9, 115,  51, 192,  18,   6, 223,
         87,  37,   9, 115,  59,  77,  64,  21,  47,
        104,  55,  44, 218,   9,  54,  53, 130, 226,
         64,  90,  70, 205,  40,  41,  23,  26,  57,
         54,  57, 112, 184,   5,  41,  38, 166, 213,
         30,  34,  26, 133, 152, 116,  10,  32, 134,
         75,  32,  12,  51, 192, 255, 160,  43,  51,
         39,  19,  53, 221,  26, 114,  32,  73, 255,
         31,   9,  65, 234,   2,  15,   1, 118,  73,
         88,  31,  35,  67, 102,  85,  55, 186,  85,
         56,  21,  23, 111,  59, 205,  45,  37, 192,
         55,  38,  70, 124,  73, 102,   1,  34,  98,
        102,  61,  71,  37,  34,  53,  31, 243, 192,
         69,  60,  71,  38,  73, 119,  28, 222,  37,
         68,  45, 128,  34,   1,  47,  11, 245, 171,
         62,  17,  19,  70, 146,  85,  55,  62,  70,
         75,  15,   9,   9,  64, 255, 184, 119,  16,
         37,  43,  37, 154, 100, 163,  85, 160,   1,
         63,   9,  92, 136,  28,  64,  32, 201,  85,
         86,   6,  28,   5,  64, 255,  25, 248,   1,
         56,   8,  17, 132, 137, 255,  55, 116, 128,
         58,  15,  20,  82, 135,  57,  26, 121,  40,
        164,  50,  31, 137, 154, 133,  25,  35, 218,
         51, 103,  44, 131, 131, 123,  31,   6, 158,
         86,  40,  64, 135, 148, 224,  45, 183, 128,
         22,  26,  17, 131, 240, 154,  14,   1, 209,
         83,  12,  13,  54, 192, 255,  68,  47,  28,
         45,  16,  21,  91,  64, 222,   7,   1, 197,
         56,  21,  39, 155,  60, 138,  23, 102, 213,
         85,  26,  85,  85, 128, 128,  32, 146, 171,
         18,  11,   7,  63, 144, 171,   4,   4, 246,
         35,  27,  10, 146, 174, 171,  12,  26, 128,
        190,  80,  35,  99, 180,  80, 126,  54,  45,
         85, 126,  47,  87, 176,  51,  41,  20,  32,
        101,  75, 128, 139, 118, 146, 116, 128,  85,
         56,  41,  15, 176, 236,  85,  37,   9,  62,
        146,  36,  19,  30, 171, 255,  97,  27,  20,
         71,  30,  17, 119, 118, 255,  17,  18, 138,
        101,  38,  60, 138,  55,  70,  43,  26, 142,
        138,  45,  61,  62, 219,   1,  81, 188,  64,
         32,  41,  20, 117, 151, 142,  20,  21, 163,
        112,  19,  12,  61, 195, 128,  48,   4,  24,
    };

    const int kDcTable[ 128 ] =
    {
          4,   5,   6,   7,   8,   9,  10,  10,  11,  12,  13,  14,  15,  16,  17,  17,
         18,  19,  20,  20,  21,  21,  22,  22,  23,  23,  24,  25,  25,  26,  27,  28,
         29,  30,  31,  32,  33,  34,  35,  36,  37,  37,  38,  39,  40,  41,  42,  43,
         44,  45,  46,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,
         59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
         75,  76,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
         91,  93,  95,  96,  98, 100, 101, 102, 104, 106, 108, 110, 112, 114, 116, 118,
        122, 124, 126, 128, 130, 132, 134, 136, 138, 140, 143, 145, 148, 151, 154, 157
    };

    const int kAcTable[ 128 ] =
    {
          4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,
         20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,
         36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,
         52,  53,  54,  55,  56,  57,  58,  60,  62,  64,  66,  68,  70,  72,  74,  76,
         78,  80,  82,  84,  86,  88,  90,  92,  94,  96,  98, 100, 102, 104, 106, 108,
        110, 112, 114, 116, 119, 122, 125, 128, 131, 134, 137, 140, 143, 146, 149, 152,
        155, 158, 161, 164, 167, 170, 173, 177, 181, 185, 189, 193, 197, 201, 205, 209,
        213, 217, 221, 225, 229, 234, 239, 245, 249, 254, 259, 264, 269, 274, 279, 284
    };

        // The RFC's trees: a leaf is stored as minus its value.
    const signed char kKeyYModeTree[ 8 ] = { -4, 2, 4, 6, -0, -1, -2, -3 };        // B_PRED, DC, V, H, TM
    const BYTE        kKeyYModeProbs[ 4 ] = { 145, 156, 163, 128 };
    const signed char kUvModeTree[ 6 ] = { -0, 2, -1, 4, -2, -3 };
    const BYTE        kKeyUvModeProbs[ 3 ] = { 142, 114, 183 };
    const signed char kBModeTree[ 18 ] = { -0, 2, -1, 4, -2, 6, 8, 12, -3, 10, -5, -6, -4, 14, -7, 16, -8, -9 };
    const signed char kSegmentTree[ 6 ] = { 2, 4, -0, -1, -2, -3 };
    const signed char kCoeffTree[ 22 ] = { -11, 2, -0, 4, -1, 6, 8, 12, -2, 10, -3, -4, 14, 16, -5, -6, 18, 20, -7, -8, -9, -10 };
        // Tokens: 0-4 the values 0-4, 5-10 categories 1-6, 11 the end of block.

    const BYTE kBands[ 17 ] = { 0, 1, 2, 3, 6, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 0 };
    const int  kZigzag4[ 16 ] = { 0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15 };
    const BYTE kCat1[] = { 159, 0 }, kCat2[] = { 165, 145, 0 }, kCat3[] = { 173, 148, 140, 0 };
    const BYTE kCat4[] = { 176, 155, 140, 135, 0 }, kCat5[] = { 180, 157, 141, 134, 130, 0 };
    const BYTE kCat6[] = { 254, 254, 243, 230, 196, 177, 153, 140, 133, 130, 129, 0 };
    const BYTE *const kCategories[ 6 ] = { kCat1, kCat2, kCat3, kCat4, kCat5, kCat6 };
    const int  kCategoryBase[ 6 ] = { 5, 7, 11, 19, 35, 67 };

    enum { B_DC_PRED, B_TM_PRED, B_VE_PRED, B_HE_PRED, B_LD_PRED, B_RD_PRED, B_VR_PRED, B_VL_PRED, B_HD_PRED, B_HU_PRED };
    enum { DC_PRED, V_PRED, H_PRED, TM_PRED, B_PRED };

        // RFC 6386's boolean entropy decoder.
    class CBoolDecoder
    {
        private:
            const BYTE *m_p;
            const BYTE *m_pEnd;
            uint32_t    m_nValue;
            uint32_t    m_nRange;
            int         m_nCount;
            int         m_nOver;

            uint32_t Next( void )
            {
                if ( m_p < m_pEnd )
                    return *m_p++;
                m_nOver++;
                return 0;
            }

        public:
            CBoolDecoder( void ) : m_p( NULL ), m_pEnd( NULL ), m_nValue( 0 ), m_nRange( 255 ), m_nCount( 0 ), m_nOver( 0 ) {}

            void Init( const BYTE *p, size_t nSize )
            {
                m_p = p;
                m_pEnd = p + nSize;
                m_nValue = Next() << 8;
                m_nValue |= Next();
                m_nRange = 255;
                m_nCount = 0;
                m_nOver = 0;
            }

            int Bool( int nProbability )
            {
                uint32_t nSplit = 1 + ( ( ( m_nRange - 1 ) * (uint32_t)nProbability ) >> 8 );
                uint32_t nBigSplit = nSplit << 8;
                int      nBit;

                if ( m_nValue >= nBigSplit )
                {
                    nBit = 1;
                    m_nRange -= nSplit;
                    m_nValue -= nBigSplit;
                }
                else
                {
                    nBit = 0;
                    m_nRange = nSplit;
                }
                while ( m_nRange < 128 )
                {
                    m_nValue <<= 1;
                    m_nRange <<= 1;
                    if ( ++m_nCount == 8 )
                    {
                        m_nCount = 0;
                        m_nValue |= Next();
                    }
                }
                return nBit;
            }

            int Literal( int nBits )
            {
                int nValue = 0;

                while ( nBits-- > 0 )
                    nValue = ( nValue << 1 ) | Bool( 128 );
                return nValue;
            }

            int Signed( int nBits )
            {
                int nValue = Literal( nBits );

                return Bool( 128 ) ? -nValue : nValue;
            }

                // A flag, then a signed value if it's set.
            int Optional( int nBits ) { return Bool( 128 ) ? Signed( nBits ) : 0; }

            int Tree( const signed char *pTree, const BYTE *pProbs, int nStart = 0 )
            {
                int i = nStart;

                while ( ( i = pTree[ i + Bool( pProbs[ i >> 1 ] ) ] ) > 0 )
                    ;
                return -i;
            }

            BOOL Overrun( void ) const { return m_nOver > 8; }
    };

    struct SQuant
    {
        int anY1[ 2 ], anY2[ 2 ], anUv[ 2 ];   // DC, AC
    };

    struct SMacroblock
    {
        int     nSegment;
        BOOL    bSkip;          // no coefficients at all, for the loop filter
        BOOL    bSubblocks;     // B_PRED
        int     nYMode;
        int     nUvMode;
        BYTE    anModes[ 16 ];
    };

    inline int Avg3( int a, int b, int c ) { return ( a + 2 * b + c + 2 ) >> 2; }
    inline int Avg2( int a, int b ) { return ( a + b + 1 ) >> 1; }

        // libwebp's 4x4 inverse DCT, added to the prediction in dst.
    void TransformAdd( const short *in, BYTE *dst, int nStride )
    {
        int C[ 16 ];
        int *tmp = C;

        for ( int i = 0; i < 4; i++ )
        {
            int a = in[ 0 ] + in[ 8 ];
            int b = in[ 0 ] - in[ 8 ];
            int c = ( ( in[ 4 ] * 35468 ) >> 16 ) - ( ( ( in[ 12 ] * 20091 ) >> 16 ) + in[ 12 ] );
            int d = ( ( ( in[ 4 ] * 20091 ) >> 16 ) + in[ 4 ] ) + ( ( in[ 12 ] * 35468 ) >> 16 );

            tmp[ 0 ] = a + d;
            tmp[ 1 ] = b + c;
            tmp[ 2 ] = b - c;
            tmp[ 3 ] = a - d;
            tmp += 4;
            in++;
        }
        tmp = C;
        for ( int i = 0; i < 4; i++ )
        {
            int dc = tmp[ 0 ] + 4;
            int a = dc + tmp[ 8 ];
            int b = dc - tmp[ 8 ];
            int c = ( ( tmp[ 4 ] * 35468 ) >> 16 ) - ( ( ( tmp[ 12 ] * 20091 ) >> 16 ) + tmp[ 12 ] );
            int d = ( ( ( tmp[ 4 ] * 20091 ) >> 16 ) + tmp[ 4 ] ) + ( ( tmp[ 12 ] * 35468 ) >> 16 );

            dst[ 0 ] = (BYTE)Clip255( dst[ 0 ] + ( ( a + d ) >> 3 ) );
            dst[ 1 ] = (BYTE)Clip255( dst[ 1 ] + ( ( b + c ) >> 3 ) );
            dst[ 2 ] = (BYTE)Clip255( dst[ 2 ] + ( ( b - c ) >> 3 ) );
            dst[ 3 ] = (BYTE)Clip255( dst[ 3 ] + ( ( a - d ) >> 3 ) );
            tmp++;
            dst += nStride;
        }
    }

        // The Walsh-Hadamard transform of the Y2 block into the 16 Y blocks' DCs.
    void TransformWht( const short *in, short *out )
    {
        int tmp[ 16 ];

        for ( int i = 0; i < 4; i++ )
        {
            int a0 = in[ 0 + i ] + in[ 12 + i ];
            int a1 = in[ 4 + i ] + in[ 8 + i ];
            int a2 = in[ 4 + i ] - in[ 8 + i ];
            int a3 = in[ 0 + i ] - in[ 12 + i ];

            tmp[ 0 + i ] = a0 + a1;
            tmp[ 8 + i ] = a0 - a1;
            tmp[ 4 + i ] = a3 + a2;
            tmp[ 12 + i ] = a3 - a2;
        }
        for ( int i = 0; i < 4; i++ )
        {
            int dc = tmp[ 0 + i * 4 ] + 3;
            int a0 = dc + tmp[ 3 + i * 4 ];
            int a1 = tmp[ 1 + i * 4 ] + tmp[ 2 + i * 4 ];
            int a2 = tmp[ 1 + i * 4 ] - tmp[ 2 + i * 4 ];
            int a3 = dc - tmp[ 3 + i * 4 ];

            out[ 0 ] = (short)( ( a0 + a1 ) >> 3 );
            out[ 16 ] = (short)( ( a3 + a2 ) >> 3 );
            out[ 32 ] = (short)( ( a0 - a1 ) >> 3 );
            out[ 48 ] = (short)( ( a3 - a2 ) >> 3 );
            out += 64;
        }
    }

    // --- loop filter (libwebp's dsp/dec.c, after RFC 6386 section 15) ---

    inline int SClip1( int v ) { return ( v < -128 ) ? -128 : ( v > 127 ) ? 127 : v; }
    inline int SClip2( int v ) { return ( v < -16 ) ? -16 : ( v > 15 ) ? 15 : v; }

    void DoFilter2( BYTE *p, int step )
    {
        int p1 = p[ -2 * step ], p0 = p[ -step ], q0 = p[ 0 ], q1 = p[ step ];
        int a = 3 * ( q0 - p0 ) + SClip1( p1 - q1 );
        int a1 = SClip2( ( a + 4 ) >> 3 );
        int a2 = SClip2( ( a + 3 ) >> 3 );

        p[ -step ] = (BYTE)Clip255( p0 + a2 );
        p[ 0 ] = (BYTE)Clip255( q0 - a1 );
    }

    void DoFilter4( BYTE *p, int step )
    {
        int p1 = p[ -2 * step ], p0 = p[ -step ], q0 = p[ 0 ], q1 = p[ step ];
        int a = 3 * ( q0 - p0 );
        int a1 = SClip2( ( a + 4 ) >> 3 );
        int a2 = SClip2( ( a + 3 ) >> 3 );
        int a3 = ( a1 + 1 ) >> 1;

        p[ -2 * step ] = (BYTE)Clip255( p1 + a3 );
        p[ -step ] = (BYTE)Clip255( p0 + a2 );
        p[ 0 ] = (BYTE)Clip255( q0 - a1 );
        p[ step ] = (BYTE)Clip255( q1 - a3 );
    }

    void DoFilter6( BYTE *p, int step )
    {
        int p2 = p[ -3 * step ], p1 = p[ -2 * step ], p0 = p[ -step ];
        int q0 = p[ 0 ], q1 = p[ step ], q2 = p[ 2 * step ];
        int a = SClip1( 3 * ( q0 - p0 ) + SClip1( p1 - q1 ) );
        int a1 = ( 27 * a + 63 ) >> 7;
        int a2 = ( 18 * a + 63 ) >> 7;
        int a3 = ( 9 * a + 63 ) >> 7;

        p[ -3 * step ] = (BYTE)Clip255( p2 + a3 );
        p[ -2 * step ] = (BYTE)Clip255( p1 + a2 );
        p[ -step ] = (BYTE)Clip255( p0 + a1 );
        p[ 0 ] = (BYTE)Clip255( q0 - a1 );
        p[ step ] = (BYTE)Clip255( q1 - a2 );
        p[ 2 * step ] = (BYTE)Clip255( q2 - a3 );
    }

    inline BOOL Hev( const BYTE *p, int step, int thresh )
    {
        return abs( p[ -2 * step ] - p[ -step ] ) > thresh || abs( p[ step ] - p[ 0 ] ) > thresh;
    }

    inline BOOL NeedsFilter( const BYTE *p, int step, int t )
    {
        return ( 4 * abs( p[ -step ] - p[ 0 ] ) + abs( p[ -2 * step ] - p[ step ] ) ) <= t;
    }

    inline BOOL NeedsFilter2( const BYTE *p, int step, int t, int it )
    {
        int p3 = p[ -4 * step ], p2 = p[ -3 * step ], p1 = p[ -2 * step ], p0 = p[ -step ];
        int q0 = p[ 0 ], q1 = p[ step ], q2 = p[ 2 * step ], q3 = p[ 3 * step ];

        if ( ( 4 * abs( p0 - q0 ) + abs( p1 - q1 ) ) > t )
            return FALSE;
        return abs( p3 - p2 ) <= it && abs( p2 - p1 ) <= it && abs( p1 - p0 ) <= it &&
               abs( q3 - q2 ) <= it && abs( q2 - q1 ) <= it && abs( q1 - q0 ) <= it;
    }

    void SimpleFilter( BYTE *p, int hstride, int vstride, int nThresh )
    {
        int nThresh2 = 2 * nThresh + 1;

        for ( int i = 0; i < 16; i++ )
            if ( NeedsFilter( p + i * vstride, hstride, nThresh2 ) )
                DoFilter2( p + i * vstride, hstride );
    }

    void FilterLoop( BYTE *p, int hstride, int vstride, int nSize, int nThresh, int nInner, int nHev, BOOL bEdge )
    {
        int nThresh2 = 2 * nThresh + 1;

        while ( nSize-- > 0 )
        {
            if ( NeedsFilter2( p, hstride, nThresh2, nInner ) )
            {
                if ( Hev( p, hstride, nHev ) )
                    DoFilter2( p, hstride );
                else if ( bEdge )
                    DoFilter6( p, hstride );
                else
                    DoFilter4( p, hstride );
            }
            p += vstride;
        }
    }

    class CLossy
    {
        private:
            const BYTE                 *m_pData;
            size_t                      m_nSize;
            std::string                 m_strError;
            int                         m_nWidth, m_nHeight;
            int                         m_nMbWide, m_nMbHigh;
            CBoolDecoder                m_oHeader;
            std::vector<CBoolDecoder>   m_aoParts;
            BOOL                        m_bSegments, m_bUpdateMap, m_bAbsolute;
            int                         m_anSegmentQuant[ 4 ], m_anSegmentFilter[ 4 ];
            BYTE                        m_anSegmentProbs[ 3 ];
            BOOL                        m_bSimple;
            int                         m_nLevel, m_nSharpness;
            BOOL                        m_bDeltas;
            int                         m_anRefDelta[ 4 ], m_anModeDelta[ 4 ];
            SQuant                      m_aoQuant[ 4 ];
            BYTE                        m_anProbs[ 4 ][ 8 ][ 3 ][ 11 ];
            BOOL                        m_bSkipProbability;
            int                         m_nSkipProbability;
            std::vector<SMacroblock>    m_aoBlocks;
            int                         m_nYStride, m_nUvStride;

            BOOL Fail( const std::string &strWhy )
            {
                if ( m_strError.empty() )
                    m_strError = strWhy;
                return FALSE;
            }

            int  Coefficients( CBoolDecoder &oBits, int nType, int nContext, const int *pQuant, int nFirst, short *pOut );
            void Predict4( BYTE *dst, int nStride, int nMode );
            void Filter( std::vector<BYTE> &anY, std::vector<BYTE> &anU, std::vector<BYTE> &anV );

        public:
            std::vector<BYTE>           m_anY, m_anU, m_anV;

            CLossy( const BYTE *p, size_t nSize ) : m_pData( p ), m_nSize( nSize ), m_nWidth( 0 ), m_nHeight( 0 ), m_nMbWide( 0 ), m_nMbHigh( 0 ),
                m_bSegments( FALSE ), m_bUpdateMap( FALSE ), m_bAbsolute( FALSE ), m_bSimple( FALSE ), m_nLevel( 0 ), m_nSharpness( 0 ),
                m_bDeltas( FALSE ), m_bSkipProbability( FALSE ), m_nSkipProbability( 0 ), m_nYStride( 0 ), m_nUvStride( 0 ) {}

            const std::string &Error( void ) const { return m_strError; }
            int Width( void ) const { return m_nWidth; }
            int Height( void ) const { return m_nHeight; }
            int YStride( void ) const { return m_nYStride; }
            int UvStride( void ) const { return m_nUvStride; }

            BOOL Decode( void );
    };

        // A block's coefficients, dequantized into pOut in natural order;
        // returns the index after the last token (as libwebp does), so
        // the block has coefficients when that's more than nFirst.
    int CLossy::Coefficients( CBoolDecoder &oBits, int nType, int nContext, const int *pQuant, int nFirst, short *pOut )
    {
        int  n = nFirst;
        BOOL bAfterZero = FALSE;

        while ( n < 16 )
        {
            const BYTE *p = m_anProbs[ nType ][ kBands[ n ] ][ nContext ];
            int         nToken = oBits.Tree( kCoeffTree, p, bAfterZero ? 2 : 0 );

            if ( nToken == 11 )
                break;
            if ( nToken == 0 )
            {
                nContext = 0;
                bAfterZero = TRUE;
                n++;
                continue;
            }

            int nValue;

            if ( nToken <= 4 )
                nValue = nToken;
            else
            {
                const BYTE *pExtra = kCategories[ nToken - 5 ];

                nValue = 0;
                for ( ; *pExtra; pExtra++ )
                    nValue = nValue + nValue + oBits.Bool( *pExtra );
                nValue += kCategoryBase[ nToken - 5 ];
            }
            if ( oBits.Bool( 128 ) )
                nValue = -nValue;
            pOut[ kZigzag4[ n ] ] = (short)( nValue * pQuant[ n > 0 ] );
            nContext = ( nToken == 1 ) ? 1 : 2;
            bAfterZero = FALSE;
            n++;
        }
        return bAfterZero ? 16 : n;
    }

    void CLossy::Predict4( BYTE *dst, int nStride, int nMode )
    {
        const BYTE *top = dst - nStride;
        int         X = top[ -1 ], A = top[ 0 ], B = top[ 1 ], C = top[ 2 ], D = top[ 3 ];
        int         E = top[ 4 ], F = top[ 5 ], G = top[ 6 ], H = top[ 7 ];
        int         I = dst[ -1 ], J = dst[ nStride - 1 ], K = dst[ 2 * nStride - 1 ], L = dst[ 3 * nStride - 1 ];

        auto Put = [&]( int x, int y, int v ) { dst[ x + y * nStride ] = (BYTE)v; };

        switch ( nMode )
        {
            case B_DC_PRED:
            {
                int dc = ( A + B + C + D + I + J + K + L + 4 ) >> 3;

                for ( int y = 0; y < 4; y++ )
                    for ( int x = 0; x < 4; x++ )
                        Put( x, y, dc );
                break;
            }
            case B_TM_PRED:
            {
                const int anTop[ 4 ] = { A, B, C, D }, anLeft[ 4 ] = { I, J, K, L };

                for ( int y = 0; y < 4; y++ )
                    for ( int x = 0; x < 4; x++ )
                        Put( x, y, Clip255( anTop[ x ] + anLeft[ y ] - X ) );
                break;
            }
            case B_VE_PRED:
            {
                const int anValues[ 4 ] = { Avg3( X, A, B ), Avg3( A, B, C ), Avg3( B, C, D ), Avg3( C, D, E ) };

                for ( int y = 0; y < 4; y++ )
                    for ( int x = 0; x < 4; x++ )
                        Put( x, y, anValues[ x ] );
                break;
            }
            case B_HE_PRED:
            {
                const int anValues[ 4 ] = { Avg3( X, I, J ), Avg3( I, J, K ), Avg3( J, K, L ), Avg3( K, L, L ) };

                for ( int y = 0; y < 4; y++ )
                    for ( int x = 0; x < 4; x++ )
                        Put( x, y, anValues[ y ] );
                break;
            }
            case B_RD_PRED:
                Put( 0, 3, Avg3( J, K, L ) );
                Put( 1, 3, Avg3( I, J, K ) ); Put( 0, 2, Avg3( I, J, K ) );
                Put( 2, 3, Avg3( X, I, J ) ); Put( 1, 2, Avg3( X, I, J ) ); Put( 0, 1, Avg3( X, I, J ) );
                Put( 3, 3, Avg3( A, X, I ) ); Put( 2, 2, Avg3( A, X, I ) ); Put( 1, 1, Avg3( A, X, I ) ); Put( 0, 0, Avg3( A, X, I ) );
                Put( 3, 2, Avg3( B, A, X ) ); Put( 2, 1, Avg3( B, A, X ) ); Put( 1, 0, Avg3( B, A, X ) );
                Put( 3, 1, Avg3( C, B, A ) ); Put( 2, 0, Avg3( C, B, A ) );
                Put( 3, 0, Avg3( D, C, B ) );
                break;
            case B_LD_PRED:
                Put( 0, 0, Avg3( A, B, C ) );
                Put( 1, 0, Avg3( B, C, D ) ); Put( 0, 1, Avg3( B, C, D ) );
                Put( 2, 0, Avg3( C, D, E ) ); Put( 1, 1, Avg3( C, D, E ) ); Put( 0, 2, Avg3( C, D, E ) );
                Put( 3, 0, Avg3( D, E, F ) ); Put( 2, 1, Avg3( D, E, F ) ); Put( 1, 2, Avg3( D, E, F ) ); Put( 0, 3, Avg3( D, E, F ) );
                Put( 3, 1, Avg3( E, F, G ) ); Put( 2, 2, Avg3( E, F, G ) ); Put( 1, 3, Avg3( E, F, G ) );
                Put( 3, 2, Avg3( F, G, H ) ); Put( 2, 3, Avg3( F, G, H ) );
                Put( 3, 3, Avg3( G, H, H ) );
                break;
            case B_VR_PRED:
                Put( 0, 0, Avg2( X, A ) ); Put( 1, 2, Avg2( X, A ) );
                Put( 1, 0, Avg2( A, B ) ); Put( 2, 2, Avg2( A, B ) );
                Put( 2, 0, Avg2( B, C ) ); Put( 3, 2, Avg2( B, C ) );
                Put( 3, 0, Avg2( C, D ) );
                Put( 0, 3, Avg3( K, J, I ) );
                Put( 0, 2, Avg3( J, I, X ) );
                Put( 0, 1, Avg3( I, X, A ) ); Put( 1, 3, Avg3( I, X, A ) );
                Put( 1, 1, Avg3( X, A, B ) ); Put( 2, 3, Avg3( X, A, B ) );
                Put( 2, 1, Avg3( A, B, C ) ); Put( 3, 3, Avg3( A, B, C ) );
                Put( 3, 1, Avg3( B, C, D ) );
                break;
            case B_VL_PRED:
                Put( 0, 0, Avg2( A, B ) );
                Put( 1, 0, Avg2( B, C ) ); Put( 0, 2, Avg2( B, C ) );
                Put( 2, 0, Avg2( C, D ) ); Put( 1, 2, Avg2( C, D ) );
                Put( 3, 0, Avg2( D, E ) ); Put( 2, 2, Avg2( D, E ) );
                Put( 0, 1, Avg3( A, B, C ) );
                Put( 1, 1, Avg3( B, C, D ) ); Put( 0, 3, Avg3( B, C, D ) );
                Put( 2, 1, Avg3( C, D, E ) ); Put( 1, 3, Avg3( C, D, E ) );
                Put( 3, 1, Avg3( D, E, F ) ); Put( 2, 3, Avg3( D, E, F ) );
                Put( 3, 2, Avg3( E, F, G ) );
                Put( 3, 3, Avg3( F, G, H ) );
                break;
            case B_HD_PRED:
                Put( 0, 0, Avg2( I, X ) ); Put( 2, 1, Avg2( I, X ) );
                Put( 0, 1, Avg2( J, I ) ); Put( 2, 2, Avg2( J, I ) );
                Put( 0, 2, Avg2( K, J ) ); Put( 2, 3, Avg2( K, J ) );
                Put( 0, 3, Avg2( L, K ) );
                Put( 3, 0, Avg3( A, B, C ) );
                Put( 2, 0, Avg3( X, A, B ) );
                Put( 1, 0, Avg3( I, X, A ) ); Put( 3, 1, Avg3( I, X, A ) );
                Put( 1, 1, Avg3( J, I, X ) ); Put( 3, 2, Avg3( J, I, X ) );
                Put( 1, 2, Avg3( K, J, I ) ); Put( 3, 3, Avg3( K, J, I ) );
                Put( 1, 3, Avg3( L, K, J ) );
                break;
            case B_HU_PRED:
                Put( 0, 0, Avg2( I, J ) );
                Put( 2, 0, Avg2( J, K ) ); Put( 0, 1, Avg2( J, K ) );
                Put( 2, 1, Avg2( K, L ) ); Put( 0, 2, Avg2( K, L ) );
                Put( 1, 0, Avg3( I, J, K ) );
                Put( 3, 0, Avg3( J, K, L ) ); Put( 1, 1, Avg3( J, K, L ) );
                Put( 3, 1, Avg3( K, L, L ) ); Put( 1, 2, Avg3( K, L, L ) );
                Put( 3, 2, L ); Put( 2, 2, L ); Put( 0, 3, L ); Put( 1, 3, L ); Put( 2, 3, L ); Put( 3, 3, L );
                break;
        }
    }

    BOOL CLossy::Decode( void )
    {
        if ( m_nSize < 10 )
            return Fail( "is cut off" );

        const BYTE *p = m_pData;
        uint32_t    nTag = p[ 0 ] | ( p[ 1 ] << 8 ) | ( p[ 2 ] << 16 );

        if ( nTag & 1 )
            return Fail( "holds a VP8 inter frame, not a still image" );
        if ( ( ( nTag >> 1 ) & 7 ) > 3 )
            return Fail( "has a VP8 version Ray doesn't read" );

        size_t nFirst = nTag >> 5;

        if ( p[ 3 ] != 0x9D || p[ 4 ] != 0x01 || p[ 5 ] != 0x2A )
            return Fail( "has a damaged VP8 header" );
        m_nWidth = ( p[ 6 ] | ( p[ 7 ] << 8 ) ) & 0x3FFF;
        m_nHeight = ( p[ 8 ] | ( p[ 9 ] << 8 ) ) & 0x3FFF;
        if ( m_nWidth == 0 || m_nHeight == 0 )
            return Fail( "has no size" );
        if ( (int64_t)m_nWidth * m_nHeight > kMaxPixels )
            return Fail( "is too large (Ray reads WebPs of up to 64 million pixels)" );
        p += 10;
        if ( nFirst > m_nSize - 10 )
            return Fail( "is cut off" );
        m_oHeader.Init( p, nFirst );

        CBoolDecoder &h = m_oHeader;

        h.Literal( 1 );     // color space
        h.Literal( 1 );     // clamping
        m_bSegments = h.Literal( 1 );
        for ( int s = 0; s < 4; s++ )
            m_anSegmentQuant[ s ] = m_anSegmentFilter[ s ] = 0;
        m_anSegmentProbs[ 0 ] = m_anSegmentProbs[ 1 ] = m_anSegmentProbs[ 2 ] = 255;
        if ( m_bSegments )
        {
            m_bUpdateMap = h.Literal( 1 );
            if ( h.Literal( 1 ) )
            {
                m_bAbsolute = h.Literal( 1 );
                for ( int s = 0; s < 4; s++ )
                    m_anSegmentQuant[ s ] = h.Optional( 7 );
                for ( int s = 0; s < 4; s++ )
                    m_anSegmentFilter[ s ] = h.Optional( 6 );
            }
            if ( m_bUpdateMap )
                for ( int s = 0; s < 3; s++ )
                    m_anSegmentProbs[ s ] = (BYTE)( h.Literal( 1 ) ? h.Literal( 8 ) : 255 );
        }
        m_bSimple = h.Literal( 1 );
        m_nLevel = h.Literal( 6 );
        m_nSharpness = h.Literal( 3 );
        m_bDeltas = h.Literal( 1 );
        for ( int s = 0; s < 4; s++ )
            m_anRefDelta[ s ] = m_anModeDelta[ s ] = 0;
        if ( m_bDeltas && h.Literal( 1 ) )
        {
            for ( int s = 0; s < 4; s++ )
                if ( h.Literal( 1 ) )
                    m_anRefDelta[ s ] = h.Signed( 6 );
            for ( int s = 0; s < 4; s++ )
                if ( h.Literal( 1 ) )
                    m_anModeDelta[ s ] = h.Signed( 6 );
        }

        int nParts = 1 << h.Literal( 2 );

            // The token partitions follow the first one, after their sizes.
        const BYTE *pParts = p + nFirst;
        size_t      nLeft = m_nSize - 10 - nFirst;

        if ( nLeft < (size_t)( 3 * ( nParts - 1 ) ) )
            return Fail( "is cut off" );

        const BYTE *pSizes = pParts;

        pParts += 3 * ( nParts - 1 );
        nLeft -= 3 * ( nParts - 1 );
        m_aoParts.resize( (size_t)nParts );
        for ( int n = 0; n < nParts; n++ )
        {
            size_t nPart = nLeft;

            if ( n < nParts - 1 )
            {
                nPart = pSizes[ n * 3 ] | ( pSizes[ n * 3 + 1 ] << 8 ) | ( pSizes[ n * 3 + 2 ] << 16 );
                if ( nPart > nLeft )
                    return Fail( "is cut off" );
            }
            m_aoParts[ (size_t)n ].Init( pParts, nPart );
            pParts += nPart;
            nLeft -= nPart;
        }

        int nBaseQ = h.Literal( 7 );
        int nY1Dc = h.Optional( 4 ), nY2Dc = h.Optional( 4 ), nY2Ac = h.Optional( 4 ), nUvDc = h.Optional( 4 ), nUvAc = h.Optional( 4 );

        auto Clip = []( int v, int nMax ) { return ( v < 0 ) ? 0 : ( v > nMax ) ? nMax : v; };

        for ( int s = 0; s < 4; s++ )
        {
            int q = nBaseQ;

            if ( m_bSegments )
                q = m_anSegmentQuant[ s ] + ( m_bAbsolute ? 0 : nBaseQ );

            SQuant &oQuant = m_aoQuant[ s ];

            oQuant.anY1[ 0 ] = kDcTable[ Clip( q + nY1Dc, 127 ) ];
            oQuant.anY1[ 1 ] = kAcTable[ Clip( q, 127 ) ];
            oQuant.anY2[ 0 ] = kDcTable[ Clip( q + nY2Dc, 127 ) ] * 2;
            oQuant.anY2[ 1 ] = ( kAcTable[ Clip( q + nY2Ac, 127 ) ] * 101581 ) >> 16;
            if ( oQuant.anY2[ 1 ] < 8 )
                oQuant.anY2[ 1 ] = 8;
            oQuant.anUv[ 0 ] = kDcTable[ Clip( q + nUvDc, 117 ) ];
            oQuant.anUv[ 1 ] = kAcTable[ Clip( q + nUvAc, 127 ) ];
        }
        h.Literal( 1 );     // refresh entropy probabilities: a single frame either way
        memcpy( m_anProbs, kDefaultCoeff, sizeof( m_anProbs ) );
        for ( int i = 0; i < 4; i++ )
            for ( int j = 0; j < 8; j++ )
                for ( int k = 0; k < 3; k++ )
                    for ( int l = 0; l < 11; l++ )
                        if ( h.Bool( kCoeffUpdate[ i ][ j ][ k ][ l ] ) )
                            m_anProbs[ i ][ j ][ k ][ l ] = (BYTE)h.Literal( 8 );
        m_bSkipProbability = h.Literal( 1 );
        if ( m_bSkipProbability )
            m_nSkipProbability = h.Literal( 8 );
        if ( h.Overrun() )
            return Fail( "is cut off" );

            // The frame, a macroblock at a time, with one pixel of border
            // above and to the left kept in the planes themselves.
        m_nMbWide = ( m_nWidth + 15 ) / 16;
        m_nMbHigh = ( m_nHeight + 15 ) / 16;
        m_nYStride = m_nMbWide * 16;
        m_nUvStride = m_nMbWide * 8;
        m_anY.assign( (size_t)m_nYStride * m_nMbHigh * 16, 0 );
        m_anU.assign( (size_t)m_nUvStride * m_nMbHigh * 8, 0 );
        m_anV.assign( (size_t)m_nUvStride * m_nMbHigh * 8, 0 );
        m_aoBlocks.assign( (size_t)m_nMbWide * m_nMbHigh, SMacroblock() );

        std::vector<BYTE> anAboveModes( (size_t)m_nMbWide * 4, B_DC_PRED );
        std::vector<BYTE> anAboveNz( (size_t)m_nMbWide * 9, 0 );     // 4 Y, 2 U, 2 V, 1 Y2
        const int         BPS = 32;
        BYTE              anWork[ BPS * 17 ];      // Y: row 0 is the border above; column 7 the border to the left
        BYTE              anUvWork[ 2 ][ BPS * 9 ];

        for ( int mby = 0; mby < m_nMbHigh; mby++ )
        {
            BYTE          anLeftModes[ 4 ] = { B_DC_PRED, B_DC_PRED, B_DC_PRED, B_DC_PRED };
            BYTE          anLeftNz[ 9 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            CBoolDecoder &oTokens = m_aoParts[ (size_t)( mby & ( nParts - 1 ) ) ];

            for ( int mbx = 0; mbx < m_nMbWide; mbx++ )
            {
                SMacroblock &oBlock = m_aoBlocks[ (size_t)mby * m_nMbWide + mbx ];

                oBlock.nSegment = ( m_bSegments && m_bUpdateMap ) ? h.Tree( kSegmentTree, m_anSegmentProbs ) : 0;

                BOOL bSkipped = m_bSkipProbability ? h.Bool( m_nSkipProbability ) : FALSE;

                oBlock.nYMode = h.Tree( kKeyYModeTree, kKeyYModeProbs );
                oBlock.bSubblocks = ( oBlock.nYMode == B_PRED );
                if ( oBlock.bSubblocks )
                {
                    for ( int n = 0; n < 16; n++ )
                    {
                        int nAbove = ( n < 4 ) ? anAboveModes[ (size_t)mbx * 4 + n ] : oBlock.anModes[ n - 4 ];
                        int nLeft = ( n & 3 ) ? oBlock.anModes[ n - 1 ] : anLeftModes[ n >> 2 ];

                        oBlock.anModes[ n ] = (BYTE)h.Tree( kBModeTree, kKeyBModeProbs[ nAbove ][ nLeft ] );
                    }
                }
                else
                {
                    static const BYTE kImplied[ 4 ] = { B_DC_PRED, B_VE_PRED, B_HE_PRED, B_TM_PRED };

                    memset( oBlock.anModes, kImplied[ oBlock.nYMode ], 16 );
                }
                for ( int n = 0; n < 4; n++ )
                {
                    anAboveModes[ (size_t)mbx * 4 + n ] = oBlock.anModes[ 12 + n ];
                    anLeftModes[ n ] = oBlock.anModes[ n * 4 + 3 ];
                }
                oBlock.nUvMode = h.Tree( kUvModeTree, kKeyUvModeProbs );

                    // The residuals.
                short    anCoeffs[ 25 * 16 ];
                BYTE    *pAboveNz = &anAboveNz[ (size_t)mbx * 9 ];
                BOOL     bAny = FALSE;
                const SQuant &q = m_aoQuant[ oBlock.nSegment ];

                memset( anCoeffs, 0, sizeof( anCoeffs ) );
                if ( ! bSkipped )
                {
                    int nFirst = 0, nType = 3;

                    if ( ! oBlock.bSubblocks )
                    {
                        short anDc[ 16 ];
                        int   nContext = pAboveNz[ 8 ] + anLeftNz[ 8 ];

                        memset( anDc, 0, sizeof( anDc ) );

                        int nEnd = Coefficients( oTokens, 1, nContext, q.anY2, 0, anDc );

                        pAboveNz[ 8 ] = anLeftNz[ 8 ] = (BYTE)( nEnd > 0 );
                        TransformWht( anDc, anCoeffs );
                        nFirst = 1;
                        nType = 0;
                    }
                    for ( int y = 0; y < 4; y++ )
                        for ( int x = 0; x < 4; x++ )
                        {
                            int nContext = pAboveNz[ x ] + anLeftNz[ y ];
                            int nEnd = Coefficients( oTokens, nType, nContext, q.anY1, nFirst, &anCoeffs[ ( y * 4 + x ) * 16 ] );

                            pAboveNz[ x ] = anLeftNz[ y ] = (BYTE)( nEnd > nFirst );
                        }
                    for ( int c = 0; c < 2; c++ )
                        for ( int y = 0; y < 2; y++ )
                            for ( int x = 0; x < 2; x++ )
                            {
                                int nContext = pAboveNz[ 4 + c * 2 + x ] + anLeftNz[ 4 + c * 2 + y ];
                                int nEnd = Coefficients( oTokens, 2, nContext, q.anUv, 0, &anCoeffs[ ( 16 + c * 4 + y * 2 + x ) * 16 ] );

                                pAboveNz[ 4 + c * 2 + x ] = anLeftNz[ 4 + c * 2 + y ] = (BYTE)( nEnd > 0 );
                            }
                    for ( int n = 0; n < 25 * 16 && ! bAny; n++ )
                        bAny = ( anCoeffs[ n ] != 0 );
                }
                else
                {
                    for ( int n = 0; n < 8; n++ )
                        pAboveNz[ n ] = anLeftNz[ n ] = 0;
                    if ( ! oBlock.bSubblocks )
                        pAboveNz[ 8 ] = anLeftNz[ 8 ] = 0;
                }
                oBlock.bSkip = ! bAny;

                    // The borders: 127 above the frame, 129 to its left.
                BYTE *y_dst = anWork + BPS + 8;
                BYTE *uv_dst[ 2 ] = { anUvWork[ 0 ] + BPS + 8, anUvWork[ 1 ] + BPS + 8 };
                BYTE *pY = &m_anY[ (size_t)mby * 16 * m_nYStride + (size_t)mbx * 16 ];
                BYTE *pPlanes[ 2 ] = { &m_anU[ (size_t)mby * 8 * m_nUvStride + (size_t)mbx * 8 ], &m_anV[ (size_t)mby * 8 * m_nUvStride + (size_t)mbx * 8 ] };

                for ( int j = 0; j < 16; j++ )
                    y_dst[ j * BPS - 1 ] = mbx ? pY[ j * m_nYStride - 1 ] : 129;
                for ( int c = 0; c < 2; c++ )
                    for ( int j = 0; j < 8; j++ )
                        uv_dst[ c ][ j * BPS - 1 ] = mbx ? pPlanes[ c ][ j * m_nUvStride - 1 ] : 129;
                if ( mby == 0 )
                {
                    memset( y_dst - BPS - 1, 127, 16 + 4 + 1 );
                    memset( uv_dst[ 0 ] - BPS - 1, 127, 9 );
                    memset( uv_dst[ 1 ] - BPS - 1, 127, 9 );
                }
                else
                {
                    memcpy( y_dst - BPS, pY - m_nYStride, 16 );
                    y_dst[ -BPS - 1 ] = mbx ? pY[ -m_nYStride - 1 ] : 129;
                    for ( int c = 0; c < 2; c++ )
                    {
                        memcpy( uv_dst[ c ] - BPS, pPlanes[ c ] - m_nUvStride, 8 );
                        uv_dst[ c ][ -BPS - 1 ] = mbx ? pPlanes[ c ][ -m_nUvStride - 1 ] : 129;
                    }
                }

                    // Luma.
                if ( oBlock.bSubblocks )
                {
                    BYTE *pTopRight = y_dst - BPS + 16;

                    if ( mby > 0 )
                    {
                        if ( mbx >= m_nMbWide - 1 )
                            memset( pTopRight, pY[ -m_nYStride + 15 ], 4 );
                        else
                            memcpy( pTopRight, pY - m_nYStride + 16, 4 );
                    }
                    for ( int r = 1; r < 4; r++ )
                        memcpy( pTopRight + r * 4 * BPS, pTopRight, 4 );
                    for ( int n = 0; n < 16; n++ )
                    {
                        BYTE *dst = y_dst + ( n & 3 ) * 4 + ( n >> 2 ) * 4 * BPS;

                        Predict4( dst, BPS, oBlock.anModes[ n ] );
                        TransformAdd( &anCoeffs[ n * 16 ], dst, BPS );
                    }
                }
                else
                {
                    int nMode = oBlock.nYMode;

                    if ( nMode == DC_PRED )
                    {
                        int nSum = 0, nDc;

                        if ( mbx > 0 )
                            for ( int j = 0; j < 16; j++ )
                                nSum += y_dst[ j * BPS - 1 ];
                        if ( mby > 0 )
                            for ( int j = 0; j < 16; j++ )
                                nSum += y_dst[ j - BPS ];
                        if ( mbx > 0 && mby > 0 )
                            nDc = ( nSum + 16 ) >> 5;
                        else if ( mbx > 0 || mby > 0 )
                            nDc = ( nSum + 8 ) >> 4;
                        else
                            nDc = 128;
                        for ( int j = 0; j < 16; j++ )
                            memset( y_dst + j * BPS, nDc, 16 );
                    }
                    else
                        for ( int j = 0; j < 16; j++ )
                            for ( int i = 0; i < 16; i++ )
                            {
                                int v;

                                if ( nMode == V_PRED )
                                    v = y_dst[ i - BPS ];
                                else if ( nMode == H_PRED )
                                    v = y_dst[ j * BPS - 1 ];
                                else
                                    v = Clip255( y_dst[ i - BPS ] + y_dst[ j * BPS - 1 ] - y_dst[ -BPS - 1 ] );
                                y_dst[ j * BPS + i ] = (BYTE)v;
                            }
                    for ( int n = 0; n < 16; n++ )
                        TransformAdd( &anCoeffs[ n * 16 ], y_dst + ( n & 3 ) * 4 + ( n >> 2 ) * 4 * BPS, BPS );
                }

                    // Chroma.
                for ( int c = 0; c < 2; c++ )
                {
                    BYTE *d = uv_dst[ c ];
                    int   nMode = oBlock.nUvMode;

                    if ( nMode == DC_PRED )
                    {
                        int nSum = 0, nDc;

                        if ( mbx > 0 )
                            for ( int j = 0; j < 8; j++ )
                                nSum += d[ j * BPS - 1 ];
                        if ( mby > 0 )
                            for ( int j = 0; j < 8; j++ )
                                nSum += d[ j - BPS ];
                        if ( mbx > 0 && mby > 0 )
                            nDc = ( nSum + 8 ) >> 4;
                        else if ( mbx > 0 || mby > 0 )
                            nDc = ( nSum + 4 ) >> 3;
                        else
                            nDc = 128;
                        for ( int j = 0; j < 8; j++ )
                            memset( d + j * BPS, nDc, 8 );
                    }
                    else
                        for ( int j = 0; j < 8; j++ )
                            for ( int i = 0; i < 8; i++ )
                            {
                                int v;

                                if ( nMode == V_PRED )
                                    v = d[ i - BPS ];
                                else if ( nMode == H_PRED )
                                    v = d[ j * BPS - 1 ];
                                else
                                    v = Clip255( d[ i - BPS ] + d[ j * BPS - 1 ] - d[ -BPS - 1 ] );
                                d[ j * BPS + i ] = (BYTE)v;
                            }
                    for ( int n = 0; n < 4; n++ )
                        TransformAdd( &anCoeffs[ ( 16 + c * 4 + n ) * 16 ], d + ( n & 1 ) * 4 + ( n >> 1 ) * 4 * BPS, BPS );
                }

                    // Into the frame (unfiltered: prediction reads it so).
                for ( int j = 0; j < 16; j++ )
                    memcpy( pY + j * m_nYStride, y_dst + j * BPS, 16 );
                for ( int c = 0; c < 2; c++ )
                    for ( int j = 0; j < 8; j++ )
                        memcpy( pPlanes[ c ] + j * m_nUvStride, uv_dst[ c ] + j * BPS, 8 );
            }
            if ( oTokens.Overrun() || h.Overrun() )
                return Fail( "is cut off" );
        }
        if ( m_nLevel > 0 )
            Filter( m_anY, m_anU, m_anV );
        return TRUE;
    }

    void CLossy::Filter( std::vector<BYTE> &anY, std::vector<BYTE> &anU, std::vector<BYTE> &anV )
    {
            // The strengths for each segment, with and without subblocks.
        int anLimit[ 4 ][ 2 ], anInner[ 4 ][ 2 ], anHev[ 4 ][ 2 ];

        for ( int s = 0; s < 4; s++ )
        {
            int nBase = m_nLevel;

            if ( m_bSegments )
            {
                nBase = m_anSegmentFilter[ s ];
                if ( ! m_bAbsolute )
                    nBase += m_nLevel;
            }
            for ( int i4 = 0; i4 < 2; i4++ )
            {
                int nLevel = nBase;

                if ( m_bDeltas )
                {
                    nLevel += m_anRefDelta[ 0 ];
                    if ( i4 )
                        nLevel += m_anModeDelta[ 0 ];
                }
                nLevel = ( nLevel < 0 ) ? 0 : ( nLevel > 63 ) ? 63 : nLevel;
                anLimit[ s ][ i4 ] = 0;
                anInner[ s ][ i4 ] = 0;
                anHev[ s ][ i4 ] = 0;
                if ( nLevel > 0 )
                {
                    int nInner = nLevel;

                    if ( m_nSharpness > 0 )
                    {
                        nInner >>= ( m_nSharpness > 4 ) ? 2 : 1;
                        if ( nInner > 9 - m_nSharpness )
                            nInner = 9 - m_nSharpness;
                    }
                    if ( nInner < 1 )
                        nInner = 1;
                    anInner[ s ][ i4 ] = nInner;
                    anLimit[ s ][ i4 ] = 2 * nLevel + nInner;
                    anHev[ s ][ i4 ] = ( nLevel >= 40 ) ? 2 : ( nLevel >= 15 ) ? 1 : 0;
                }
            }
        }

        int ys = m_nYStride, uvs = m_nUvStride;

        for ( int mby = 0; mby < m_nMbHigh; mby++ )
            for ( int mbx = 0; mbx < m_nMbWide; mbx++ )
            {
                const SMacroblock &oBlock = m_aoBlocks[ (size_t)mby * m_nMbWide + mbx ];
                int                i4 = oBlock.bSubblocks ? 1 : 0;
                int                nLimit = anLimit[ oBlock.nSegment ][ i4 ];

                if ( nLimit == 0 )
                    continue;

                int   nInner = anInner[ oBlock.nSegment ][ i4 ], nHev = anHev[ oBlock.nSegment ][ i4 ];
                BOOL  bInner = oBlock.bSubblocks || ! oBlock.bSkip;
                BYTE *y = &anY[ (size_t)mby * 16 * ys + (size_t)mbx * 16 ];
                BYTE *u = &anU[ (size_t)mby * 8 * uvs + (size_t)mbx * 8 ];
                BYTE *v = &anV[ (size_t)mby * 8 * uvs + (size_t)mbx * 8 ];

                if ( m_bSimple )
                {
                    if ( mbx > 0 )
                        SimpleFilter( y, 1, ys, nLimit + 4 );
                    if ( bInner )
                        for ( int k = 1; k < 4; k++ )
                            SimpleFilter( y + 4 * k, 1, ys, nLimit );
                    if ( mby > 0 )
                        SimpleFilter( y, ys, 1, nLimit + 4 );
                    if ( bInner )
                        for ( int k = 1; k < 4; k++ )
                            SimpleFilter( y + 4 * k * ys, ys, 1, nLimit );
                    continue;
                }
                if ( mbx > 0 )
                {
                    FilterLoop( y, 1, ys, 16, nLimit + 4, nInner, nHev, TRUE );
                    FilterLoop( u, 1, uvs, 8, nLimit + 4, nInner, nHev, TRUE );
                    FilterLoop( v, 1, uvs, 8, nLimit + 4, nInner, nHev, TRUE );
                }
                if ( bInner )
                {
                    for ( int k = 1; k < 4; k++ )
                        FilterLoop( y + 4 * k, 1, ys, 16, nLimit, nInner, nHev, FALSE );
                    FilterLoop( u + 4, 1, uvs, 8, nLimit, nInner, nHev, FALSE );
                    FilterLoop( v + 4, 1, uvs, 8, nLimit, nInner, nHev, FALSE );
                }
                if ( mby > 0 )
                {
                    FilterLoop( y, ys, 1, 16, nLimit + 4, nInner, nHev, TRUE );
                    FilterLoop( u, uvs, 1, 8, nLimit + 4, nInner, nHev, TRUE );
                    FilterLoop( v, uvs, 1, 8, nLimit + 4, nInner, nHev, TRUE );
                }
                if ( bInner )
                {
                    for ( int k = 1; k < 4; k++ )
                        FilterLoop( y + 4 * k * ys, ys, 1, 16, nLimit, nInner, nHev, FALSE );
                    FilterLoop( u + 4 * uvs, uvs, 1, 8, nLimit, nInner, nHev, FALSE );
                    FilterLoop( v + 4 * uvs, uvs, 1, 8, nLimit, nInner, nHev, FALSE );
                }
            }
    }

    // --- YUV 4:2:0 to RGB, as libwebp does it by default ---

    inline int MultHi( int v, int nCoeff ) { return ( v * nCoeff ) >> 8; }
    inline int Clip8( int v ) { return ( ( v & ~16383 ) == 0 ) ? ( v >> 6 ) : ( v < 0 ) ? 0 : 255; }

    inline void YuvToRgb( int y, int u, int v, BYTE *pOut )
    {
        pOut[ 0 ] = (BYTE)Clip8( MultHi( y, 19077 ) + MultHi( v, 26149 ) - 14234 );
        pOut[ 1 ] = (BYTE)Clip8( MultHi( y, 19077 ) - MultHi( u, 6419 ) - MultHi( v, 13320 ) + 8708 );
        pOut[ 2 ] = (BYTE)Clip8( MultHi( y, 19077 ) + MultHi( u, 33050 ) - 17685 );
    }

        // libwebp's "fancy" upsampler for a pair of luma rows (bottom may
        // be missing): the chroma between two chroma rows, weighted 9:3:3:1.
    void UpsamplePair( const BYTE *pTopY, const BYTE *pBottomY, const BYTE *pTopU, const BYTE *pTopV, const BYTE *pCurU, const BYTE *pCurV,
                       BYTE *pTopOut, BYTE *pBottomOut, int nLength )
    {
        int      nLastPair = ( nLength - 1 ) >> 1;
        uint32_t tl = pTopU[ 0 ] | ( (uint32_t)pTopV[ 0 ] << 16 );
        uint32_t l = pCurU[ 0 ] | ( (uint32_t)pCurV[ 0 ] << 16 );

        {
            uint32_t uv0 = ( 3 * tl + l + 0x00020002u ) >> 2;

            YuvToRgb( pTopY[ 0 ], (int)( uv0 & 0xFF ), (int)( uv0 >> 16 ), pTopOut );
        }
        if ( pBottomY )
        {
            uint32_t uv0 = ( 3 * l + tl + 0x00020002u ) >> 2;

            YuvToRgb( pBottomY[ 0 ], (int)( uv0 & 0xFF ), (int)( uv0 >> 16 ), pBottomOut );
        }
        for ( int x = 1; x <= nLastPair; x++ )
        {
            uint32_t t = pTopU[ x ] | ( (uint32_t)pTopV[ x ] << 16 );
            uint32_t uv = pCurU[ x ] | ( (uint32_t)pCurV[ x ] << 16 );
            uint32_t nAverage = tl + t + l + uv + 0x00080008u;
            uint32_t nDiag12 = ( nAverage + 2 * ( t + l ) ) >> 3;
            uint32_t nDiag03 = ( nAverage + 2 * ( tl + uv ) ) >> 3;

            {
                uint32_t uv0 = ( nDiag12 + tl ) >> 1;
                uint32_t uv1 = ( nDiag03 + t ) >> 1;

                YuvToRgb( pTopY[ 2 * x - 1 ], (int)( uv0 & 0xFF ), (int)( uv0 >> 16 ), pTopOut + ( 2 * x - 1 ) * 3 );
                YuvToRgb( pTopY[ 2 * x ], (int)( uv1 & 0xFF ), (int)( uv1 >> 16 ), pTopOut + ( 2 * x ) * 3 );
            }
            if ( pBottomY )
            {
                uint32_t uv0 = ( nDiag03 + l ) >> 1;
                uint32_t uv1 = ( nDiag12 + uv ) >> 1;

                YuvToRgb( pBottomY[ 2 * x - 1 ], (int)( uv0 & 0xFF ), (int)( uv0 >> 16 ), pBottomOut + ( 2 * x - 1 ) * 3 );
                YuvToRgb( pBottomY[ 2 * x ], (int)( uv1 & 0xFF ), (int)( uv1 >> 16 ), pBottomOut + ( 2 * x ) * 3 );
            }
            tl = t;
            l = uv;
        }
        if ( ! ( nLength & 1 ) )
        {
            {
                uint32_t uv0 = ( 3 * tl + l + 0x00020002u ) >> 2;

                YuvToRgb( pTopY[ nLength - 1 ], (int)( uv0 & 0xFF ), (int)( uv0 >> 16 ), pTopOut + ( nLength - 1 ) * 3 );
            }
            if ( pBottomY )
            {
                uint32_t uv0 = ( 3 * l + tl + 0x00020002u ) >> 2;

                YuvToRgb( pBottomY[ nLength - 1 ], (int)( uv0 & 0xFF ), (int)( uv0 >> 16 ), pBottomOut + ( nLength - 1 ) * 3 );
            }
        }
    }

    void ToRgb( const CLossy &oFrame, std::vector<BYTE> &anPixels )
    {
        int w = oFrame.Width(), h = oFrame.Height();
        int ys = oFrame.YStride(), uvs = oFrame.UvStride();
        int nChromaHigh = ( h + 1 ) / 2;

        anPixels.assign( (size_t)w * (size_t)h * 3, 0 );

        const BYTE *Y = oFrame.m_anY.data(), *U = oFrame.m_anU.data(), *V = oFrame.m_anV.data();

        UpsamplePair( Y, NULL, U, V, U, V, &anPixels[ 0 ], NULL, w );
        for ( int k = 1; 2 * k - 1 < h; k++ )
        {
            int         nCur = ( k < nChromaHigh ) ? k : nChromaHigh - 1;
            const BYTE *pBottom = ( 2 * k < h ) ? Y + (size_t)( 2 * k ) * ys : NULL;

            UpsamplePair( Y + (size_t)( 2 * k - 1 ) * ys, pBottom, U + (size_t)( k - 1 ) * uvs, V + (size_t)( k - 1 ) * uvs,
                          U + (size_t)nCur * uvs, V + (size_t)nCur * uvs, &anPixels[ (size_t)( 2 * k - 1 ) * w * 3 ],
                          pBottom ? &anPixels[ (size_t)( 2 * k ) * w * 3 ] : NULL, w );
        }
    }

    // =====================================================================
    // Alpha
    // =====================================================================

    BOOL DecodeAlpha( const BYTE *p, size_t nSize, int nWidth, int nHeight, std::vector<BYTE> &anAlpha, std::string &strError )
    {
        if ( nSize < 1 )
        {
            strError = "has an empty alpha chunk";
            return FALSE;
        }

        int    nCompression = p[ 0 ] & 3, nFilter = ( p[ 0 ] >> 2 ) & 3;
        size_t nPixels = (size_t)nWidth * (size_t)nHeight;

        anAlpha.assign( nPixels, 255 );
        if ( nCompression == 0 )
        {
            if ( nSize - 1 < nPixels )
            {
                strError = "has a cut-off alpha chunk";
                return FALSE;
            }
            memcpy( anAlpha.data(), p + 1, nPixels );
        }
        else if ( nCompression == 1 )
        {
            CLossless             oLossless( p + 1, nSize - 1 );
            std::vector<uint32_t> anArgb;

            if ( ! oLossless.Decode( nWidth, nHeight, anArgb ) )
            {
                strError = oLossless.Error() + " (in its alpha)";
                return FALSE;
            }
            for ( size_t n = 0; n < nPixels; n++ )
                anAlpha[ n ] = (BYTE)( ( anArgb[ n ] >> 8 ) & 0xFF );
        }
        else
        {
            strError = "compresses its alpha in a way Ray doesn't read";
            return FALSE;
        }
        if ( nFilter )
        {
            for ( int y = 0; y < nHeight; y++ )
                for ( int x = 0; x < nWidth; x++ )
                {
                    BYTE *a = &anAlpha[ (size_t)y * nWidth + x ];
                    int   nPredicted;

                    if ( x == 0 && y == 0 )
                        nPredicted = 0;
                    else if ( y == 0 )
                        nPredicted = a[ -1 ];
                    else if ( x == 0 )
                        nPredicted = a[ -nWidth ];
                    else if ( nFilter == 1 )
                        nPredicted = a[ -1 ];
                    else if ( nFilter == 2 )
                        nPredicted = a[ -nWidth ];
                    else
                        nPredicted = Clip255( a[ -1 ] + a[ -nWidth ] - a[ -nWidth - 1 ] );
                    *a = (BYTE)( ( *a + nPredicted ) & 0xFF );
                }
        }
        return TRUE;
    }

    inline uint32_t Little32( const BYTE *p ) { return p[ 0 ] | ( p[ 1 ] << 8 ) | ( p[ 2 ] << 16 ) | ( (uint32_t)p[ 3 ] << 24 ); }
}

BOOL CWebpImage::IsWebp( const std::string &strData )
{
    return strData.size() >= 12 && memcmp( strData.data(), "RIFF", 4 ) == 0 && memcmp( strData.data() + 8, "WEBP", 4 ) == 0;
}

BOOL CWebpImage::Decode( const std::string &strData, int &nWidth, int &nHeight, std::vector<BYTE> &anPixels, std::string &strError,
                         std::vector<BYTE> *pAlpha )
{
    nWidth = nHeight = 0;
    anPixels.clear();
    if ( ! IsWebp( strData ) )
    {
        strError = "isn't a WebP";
        return FALSE;
    }

    const BYTE *pData = (const BYTE *)strData.data();
    size_t      nSize = strData.size();
    size_t      nRiff = Little32( pData + 4 );

    if ( nRiff + 8 < nSize )
        nSize = nRiff + 8;      // trailing bytes after the RIFF are ignored

        // The chunks.
    const BYTE *pImage = NULL, *pAlphaChunk = NULL;
    size_t      nImage = 0, nAlphaChunk = 0;
    BOOL        bLossless = FALSE, bExtended = FALSE;
    int         nCanvasWidth = 0, nCanvasHeight = 0;

    for ( size_t nAt = 12; nAt + 8 <= nSize; )
    {
        const BYTE *pChunk = pData + nAt;
        size_t      nLength = Little32( pChunk + 4 );

        if ( nLength > nSize - nAt - 8 )
        {
            strError = "is cut off";
            return FALSE;
        }
        if ( memcmp( pChunk, "VP8X", 4 ) == 0 )
        {
            if ( nLength < 10 )
            {
                strError = "has a damaged VP8X chunk";
                return FALSE;
            }
            bExtended = TRUE;
            if ( pChunk[ 8 ] & 0x02 )
            {
                strError = "is an animated WebP, which Ray doesn't read";
                return FALSE;
            }
            nCanvasWidth = (int)( ( pChunk[ 12 ] | ( pChunk[ 13 ] << 8 ) | ( pChunk[ 14 ] << 16 ) ) + 1 );
            nCanvasHeight = (int)( ( pChunk[ 15 ] | ( pChunk[ 16 ] << 8 ) | ( pChunk[ 17 ] << 16 ) ) + 1 );
        }
        else if ( memcmp( pChunk, "ANIM", 4 ) == 0 || memcmp( pChunk, "ANMF", 4 ) == 0 )
        {
            strError = "is an animated WebP, which Ray doesn't read";
            return FALSE;
        }
        else if ( memcmp( pChunk, "ALPH", 4 ) == 0 && pImage == NULL )
        {
            pAlphaChunk = pChunk + 8;
            nAlphaChunk = nLength;
        }
        else if ( ( memcmp( pChunk, "VP8 ", 4 ) == 0 || memcmp( pChunk, "VP8L", 4 ) == 0 ) && pImage == NULL )
        {
            pImage = pChunk + 8;
            nImage = nLength;
            bLossless = ( pChunk[ 3 ] == 'L' );
        }
        nAt += 8 + nLength + ( nLength & 1 );
    }
    if ( pImage == NULL )
    {
        strError = "has no image data";
        return FALSE;
    }

    std::vector<BYTE> anAlpha;

    if ( bLossless )
    {
        CLossless             oLossless( pImage, nImage );
        std::vector<uint32_t> anArgb;

        if ( ! oLossless.DecodeChunk( nWidth, nHeight, anArgb ) )
        {
            strError = oLossless.Error();
            nWidth = nHeight = 0;
            return FALSE;
        }

        size_t nPixels = (size_t)nWidth * (size_t)nHeight;

        anPixels.resize( nPixels * 3 );
        anAlpha.resize( nPixels );
        for ( size_t n = 0; n < nPixels; n++ )
        {
            uint32_t nArgb = anArgb[ n ];

            anPixels[ n * 3 ] = (BYTE)( nArgb >> 16 );
            anPixels[ n * 3 + 1 ] = (BYTE)( nArgb >> 8 );
            anPixels[ n * 3 + 2 ] = (BYTE)nArgb;
            anAlpha[ n ] = (BYTE)( nArgb >> 24 );
        }
    }
    else
    {
        CLossy oLossy( pImage, nImage );

        if ( ! oLossy.Decode() )
        {
            strError = oLossy.Error();
            return FALSE;
        }
        nWidth = oLossy.Width();
        nHeight = oLossy.Height();
        ToRgb( oLossy, anPixels );
        if ( pAlphaChunk )
        {
            if ( ! DecodeAlpha( pAlphaChunk, nAlphaChunk, nWidth, nHeight, anAlpha, strError ) )
            {
                anPixels.clear();
                nWidth = nHeight = 0;
                return FALSE;
            }
        }
        else
            anAlpha.assign( (size_t)nWidth * (size_t)nHeight, 255 );
    }
    if ( bExtended && ( nCanvasWidth != nWidth || nCanvasHeight != nHeight ) )
    {
        strError = "has a canvas of a different size from its image";
        anPixels.clear();
        nWidth = nHeight = 0;
        return FALSE;
    }
    if ( pAlpha )
        pAlpha->swap( anAlpha );
    return TRUE;
}
