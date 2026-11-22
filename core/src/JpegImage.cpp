/*************************************************************************
*   CJpegImage                                                           *
*                                                                        *
*   A JPEG decoder of our own (ITU-T T.81), integer-only so every        *
*   platform gives the same pixels. See JpegImage.h.                     *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#include "JpegImage.h"

#include <cstdint>
#include <cstring>

namespace
{
        // The natural (row-major) position of each coefficient, in the
        // zigzag order the file stores them.
    const int kZigzag[ 64 ] =
    {
         0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
    };

    const int64_t kMaxPixels = (int64_t)64 * 1024 * 1024;   // 8192 x 8192
    const int   kFastBits = 9;

    struct SHuffman
    {
        BOOL            bDefined;
        unsigned short  anFast[ 1 << kFastBits ];   // (length << 8) | symbol, or 0
        int             anMaxCode[ 18 ];            // the largest code of each length, or -1
        int             anOffset[ 17 ];             // index of a length's first symbol, less its first code
        BYTE            anValues[ 256 ];

        SHuffman( void ) : bDefined( FALSE ) {}
    };

        // Builds a table from the counts of codes of each length and the
        // symbols in order. FALSE if the counts can't form a prefix code.
    BOOL BuildHuffman( SHuffman &oTable, const BYTE anCounts[ 16 ], const BYTE *pValues, int nValues )
    {
        int nCode = 0, nIndex = 0;

        memset( oTable.anFast, 0, sizeof( oTable.anFast ) );
        memcpy( oTable.anValues, pValues, (size_t)nValues );
        for ( int nLength = 1; nLength <= 16; nLength++ )
        {
            int nCount = anCounts[ nLength - 1 ];

            oTable.anOffset[ nLength ] = nIndex - nCode;
            for ( int nSymbol = 0; nSymbol < nCount; nSymbol++, nIndex++, nCode++ )
            {
                if ( nCode >= ( 1 << nLength ) )
                    return FALSE;
                if ( nLength <= kFastBits )
                {
                    int nFirst = nCode << ( kFastBits - nLength );
                    int nFill = 1 << ( kFastBits - nLength );

                    for ( int nAt = 0; nAt < nFill; nAt++ )
                        oTable.anFast[ nFirst + nAt ] = (unsigned short)( ( nLength << 8 ) | pValues[ nIndex ] );
                }
            }
            oTable.anMaxCode[ nLength ] = nCount ? nCode - 1 : -1;
            nCode <<= 1;
        }
        oTable.anMaxCode[ 17 ] = 0x7FFFFFFF;
        oTable.bDefined = TRUE;
        return TRUE;
    }

        // The entropy-coded data, read MSB first, with the 0xFF 0x00
        // stuffing removed. At a marker (or the end) it feeds zeros.
    struct SBits
    {
        const BYTE *p;
        const BYTE *pEnd;
        uint32_t    nBits;
        int         nCount;
        int         nPadding;       // zero bytes fed past the end of the data
        BOOL        bMarker;

        void Reset( const BYTE *pAt, const BYTE *pLast )
        {
            p = pAt;
            pEnd = pLast;
            nBits = 0;
            nCount = 0;
            nPadding = 0;
            bMarker = FALSE;
        }

        void Fill( void )
        {
            while ( nCount <= 24 )
            {
                uint32_t nByte = 0;

                if ( ! bMarker && p < pEnd )
                {
                    nByte = *p;
                    if ( nByte == 0xFF )
                    {
                        if ( p + 1 < pEnd && p[ 1 ] == 0 )
                            p += 2;
                        else
                        {
                            bMarker = TRUE;     // stay on the marker
                            nByte = 0;
                        }
                    }
                    else
                        p++;
                }
                else
                    nPadding++;
                nBits |= nByte << ( 24 - nCount );
                nCount += 8;
            }
        }

            /// Whether the data ran out with no marker to end it.
        BOOL CutOff( void ) const { return ! bMarker && p >= pEnd && nPadding > 4; }

        int Bit( void )
        {
            Fill();

            int nBit = (int)( nBits >> 31 );

            nBits <<= 1;
            nCount--;
            return nBit;
        }

        int Get( int nLength )
        {
            if ( nLength == 0 )
                return 0;
            Fill();

            int nValue = (int)( nBits >> ( 32 - nLength ) );

            nBits <<= nLength;
            nCount -= nLength;
            return nValue;
        }

            // A value of nLength bits, with its sign extended as T.81's
            // EXTEND procedure says.
        int Extend( int nLength )
        {
            if ( nLength == 0 )
                return 0;

            int nValue = Get( nLength );

            if ( nValue < ( 1 << ( nLength - 1 ) ) )
                nValue -= ( 1 << nLength ) - 1;
            return nValue;
        }

            // The next symbol, or -1 if the bits match no code.
        int Decode( const SHuffman &oTable )
        {
            Fill();

            unsigned short nFast = oTable.anFast[ nBits >> ( 32 - kFastBits ) ];

            if ( nFast )
            {
                int nLength = nFast >> 8;

                nBits <<= nLength;
                nCount -= nLength;
                return nFast & 0xFF;
            }
            for ( int nLength = kFastBits + 1; nLength <= 16; nLength++ )
            {
                int nCode = (int)( nBits >> ( 32 - nLength ) );

                if ( nCode <= oTable.anMaxCode[ nLength ] )
                {
                    int nIndex = oTable.anOffset[ nLength ] + nCode;

                    nBits <<= nLength;
                    nCount -= nLength;
                    return ( nIndex >= 0 && nIndex < 256 ) ? oTable.anValues[ nIndex ] : -1;
                }
            }
            return -1;
        }
    };

    struct SComponent
    {
        int                     nId;
        int                     nH, nV;             // sampling factors
        int                     nQuant;             // quantization table
        int                     nDcTable, nAcTable;
        int                     nWidth, nHeight;    // its samples, as stored (before upsampling)
        int                     nBlocksWide;        // blocks a row, padded to whole MCUs
        int                     nBlocksHigh;
        int                     nPredictor;         // the DC value of the last block
        std::vector<short>      anCoefficients;     // 64 a block, natural order, not dequantized
        std::vector<BYTE>       anSamples;          // nBlocksWide * 8 wide

        short *Block( int nX, int nY ) { return &anCoefficients[ ( (size_t)nY * (size_t)nBlocksWide + (size_t)nX ) * 64 ]; }
    };

    // ---------------------------------------------------------------------
    // The IJG's accurate integer inverse DCT (jidctint.c), with its
    // 13-bit constants. Results are clamped to 0-255.

    const int kConstBits = 13;
    const int kPass1Bits = 2;

    inline int Descale( int64_t nValue, int nShift ) { return (int)( ( nValue + ( (int64_t)1 << ( nShift - 1 ) ) ) >> nShift ); }
    inline BYTE Clamp( int nValue ) { return (BYTE)( ( nValue < 0 ) ? 0 : ( nValue > 255 ) ? 255 : nValue ); }

    void InverseDct( const short *pIn, const unsigned short *pQuant, BYTE *pOut, int nStride )
    {
        int64_t anWork[ 64 ];

        for ( int nColumn = 0; nColumn < 8; nColumn++ )
        {
            const short          *in = pIn + nColumn;
            const unsigned short *q = pQuant + nColumn;
            int64_t                 *ws = anWork + nColumn;

            if ( in[ 8 ] == 0 && in[ 16 ] == 0 && in[ 24 ] == 0 && in[ 32 ] == 0 && in[ 40 ] == 0 && in[ 48 ] == 0 && in[ 56 ] == 0 )
            {
                int64_t nDc = ( (int64_t)in[ 0 ] * q[ 0 ] ) * ( (int64_t)1 << kPass1Bits );

                for ( int nRow = 0; nRow < 8; nRow++ )
                    ws[ nRow * 8 ] = nDc;
                continue;
            }

            int64_t z2 = (int64_t)in[ 16 ] * q[ 16 ], z3 = (int64_t)in[ 48 ] * q[ 48 ];
            int64_t z1 = ( z2 + z3 ) * 4433;
            int64_t tmp2 = z1 + z3 * -15137;
            int64_t tmp3 = z1 + z2 * 6270;

            z2 = (int64_t)in[ 0 ] * q[ 0 ];
            z3 = (int64_t)in[ 32 ] * q[ 32 ];

            int64_t tmp0 = ( z2 + z3 ) * ( (int64_t)1 << kConstBits );
            int64_t tmp1 = ( z2 - z3 ) * ( (int64_t)1 << kConstBits );
            int64_t tmp10 = tmp0 + tmp3, tmp13 = tmp0 - tmp3, tmp11 = tmp1 + tmp2, tmp12 = tmp1 - tmp2;

            tmp0 = (int64_t)in[ 56 ] * q[ 56 ];
            tmp1 = (int64_t)in[ 40 ] * q[ 40 ];
            tmp2 = (int64_t)in[ 24 ] * q[ 24 ];
            tmp3 = (int64_t)in[ 8 ] * q[ 8 ];

            int64_t z11 = tmp0 + tmp3, z12 = tmp1 + tmp2, z13 = tmp0 + tmp2, z14 = tmp1 + tmp3;
            int64_t z5 = ( z13 + z14 ) * 9633;

            tmp0 *= 2446;
            tmp1 *= 16819;
            tmp2 *= 25172;
            tmp3 *= 12299;
            z11 *= -7373;
            z12 *= -20995;
            z13 = z13 * -16069 + z5;
            z14 = z14 * -3196 + z5;
            tmp0 += z11 + z13;
            tmp1 += z12 + z14;
            tmp2 += z12 + z13;
            tmp3 += z11 + z14;

            ws[ 0 ] = Descale( tmp10 + tmp3, kConstBits - kPass1Bits );
            ws[ 56 ] = Descale( tmp10 - tmp3, kConstBits - kPass1Bits );
            ws[ 8 ] = Descale( tmp11 + tmp2, kConstBits - kPass1Bits );
            ws[ 48 ] = Descale( tmp11 - tmp2, kConstBits - kPass1Bits );
            ws[ 16 ] = Descale( tmp12 + tmp1, kConstBits - kPass1Bits );
            ws[ 40 ] = Descale( tmp12 - tmp1, kConstBits - kPass1Bits );
            ws[ 24 ] = Descale( tmp13 + tmp0, kConstBits - kPass1Bits );
            ws[ 32 ] = Descale( tmp13 - tmp0, kConstBits - kPass1Bits );
        }
        for ( int nRow = 0; nRow < 8; nRow++ )
        {
            const int64_t *ws = anWork + nRow * 8;
            BYTE       *out = pOut + nRow * nStride;
            const int   nShift = kConstBits + kPass1Bits + 3;

            int64_t z2 = ws[ 2 ], z3 = ws[ 6 ];
            int64_t z1 = ( z2 + z3 ) * 4433;
            int64_t tmp2 = z1 + z3 * -15137;
            int64_t tmp3 = z1 + z2 * 6270;
            int64_t tmp0 = ( ws[ 0 ] + ws[ 4 ] ) * ( (int64_t)1 << kConstBits );
            int64_t tmp1 = ( ws[ 0 ] - ws[ 4 ] ) * ( (int64_t)1 << kConstBits );
            int64_t tmp10 = tmp0 + tmp3, tmp13 = tmp0 - tmp3, tmp11 = tmp1 + tmp2, tmp12 = tmp1 - tmp2;

            tmp0 = ws[ 7 ];
            tmp1 = ws[ 5 ];
            tmp2 = ws[ 3 ];
            tmp3 = ws[ 1 ];

            int64_t z11 = tmp0 + tmp3, z12 = tmp1 + tmp2, z13 = tmp0 + tmp2, z14 = tmp1 + tmp3;
            int64_t z5 = ( z13 + z14 ) * 9633;

            tmp0 *= 2446;
            tmp1 *= 16819;
            tmp2 *= 25172;
            tmp3 *= 12299;
            z11 *= -7373;
            z12 *= -20995;
            z13 = z13 * -16069 + z5;
            z14 = z14 * -3196 + z5;
            tmp0 += z11 + z13;
            tmp1 += z12 + z14;
            tmp2 += z12 + z13;
            tmp3 += z11 + z14;

            out[ 0 ] = Clamp( Descale( tmp10 + tmp3, nShift ) + 128 );
            out[ 7 ] = Clamp( Descale( tmp10 - tmp3, nShift ) + 128 );
            out[ 1 ] = Clamp( Descale( tmp11 + tmp2, nShift ) + 128 );
            out[ 6 ] = Clamp( Descale( tmp11 - tmp2, nShift ) + 128 );
            out[ 2 ] = Clamp( Descale( tmp12 + tmp1, nShift ) + 128 );
            out[ 5 ] = Clamp( Descale( tmp12 - tmp1, nShift ) + 128 );
            out[ 3 ] = Clamp( Descale( tmp13 + tmp0, nShift ) + 128 );
            out[ 4 ] = Clamp( Descale( tmp13 - tmp0, nShift ) + 128 );
        }
    }

    // ---------------------------------------------------------------------

    class CDecoder
    {
        private:
            const BYTE             *m_pData;
            size_t                  m_nSize;
            unsigned short          m_aanQuant[ 4 ][ 64 ];  // natural order
            BOOL                    m_abQuant[ 4 ];
            SHuffman                m_aoDc[ 4 ];
            SHuffman                m_aoAc[ 4 ];
            std::vector<SComponent> m_aoComponents;
            BOOL                    m_bFrame;
            BOOL                    m_bProgressive;
            int                     m_nWidth, m_nHeight;
            int                     m_nMaxH, m_nMaxV;
            int                     m_nMcusWide, m_nMcusHigh;
            int                     m_nRestart;
            int                     m_nOrientation;
            int                     m_nAdobe;               // Adobe transform flag, or -1
            int                     m_nEobRun;
            SBits                   m_oBits;
            std::string             m_strError;

            BOOL Fail( const std::string &strWhy )
            {
                if ( m_strError.empty() )
                    m_strError = strWhy;
                return FALSE;
            }

            static int Big16( const BYTE *p ) { return ( p[ 0 ] << 8 ) | p[ 1 ]; }

            BOOL ReadQuant( const BYTE *p, int nLength );
            BOOL ReadHuffman( const BYTE *p, int nLength );
            BOOL ReadFrame( const BYTE *p, int nLength );
            void ReadExif( const BYTE *p, int nLength );
            BOOL ReadScan( const BYTE *p, int nLength, size_t &nEnd );
            BOOL Restart( void );
            BOOL DecodeBlock( SComponent &oComponent, short *pBlock, int nStart, int nEnd, int nHigh, int nLow );
            BOOL Finish( int &nWidth, int &nHeight, std::vector<BYTE> &anPixels );
            void Upsample( const SComponent &oComponent, std::vector<BYTE> &anOut ) const;

        public:
            CDecoder( const std::string &strData ) :
                m_pData( (const BYTE *)strData.data() ),
                m_nSize( strData.size() ),
                m_bFrame( FALSE ),
                m_bProgressive( FALSE ),
                m_nWidth( 0 ),
                m_nHeight( 0 ),
                m_nMaxH( 1 ),
                m_nMaxV( 1 ),
                m_nMcusWide( 0 ),
                m_nMcusHigh( 0 ),
                m_nRestart( 0 ),
                m_nOrientation( 1 ),
                m_nAdobe( -1 ),
                m_nEobRun( 0 )
            {
                memset( m_abQuant, 0, sizeof( m_abQuant ) );
            }

            BOOL Run( int &nWidth, int &nHeight, std::vector<BYTE> &anPixels, std::string &strError );
    };

    BOOL CDecoder::ReadQuant( const BYTE *p, int nLength )
    {
        while ( nLength > 0 )
        {
            int nPrecision = p[ 0 ] >> 4, nTable = p[ 0 ] & 15;
            int nBytes = 1 + 64 * ( nPrecision ? 2 : 1 );

            if ( nTable > 3 || nPrecision > 1 || nLength < nBytes )
                return Fail( "has a damaged quantization table" );
            for ( int nIndex = 0; nIndex < 64; nIndex++ )
            {
                int nValue = nPrecision ? Big16( p + 1 + nIndex * 2 ) : p[ 1 + nIndex ];

                m_aanQuant[ nTable ][ kZigzag[ nIndex ] ] = (unsigned short)nValue;
            }
            m_abQuant[ nTable ] = TRUE;
            p += nBytes;
            nLength -= nBytes;
        }
        return TRUE;
    }

    BOOL CDecoder::ReadHuffman( const BYTE *p, int nLength )
    {
        while ( nLength > 0 )
        {
            if ( nLength < 17 )
                return Fail( "has a damaged Huffman table" );

            int nClass = p[ 0 ] >> 4, nTable = p[ 0 ] & 15, nValues = 0;

            for ( int nIndex = 0; nIndex < 16; nIndex++ )
                nValues += p[ 1 + nIndex ];
            if ( nClass > 1 || nTable > 3 || nValues > 256 || nLength < 17 + nValues )
                return Fail( "has a damaged Huffman table" );
            if ( ! BuildHuffman( nClass ? m_aoAc[ nTable ] : m_aoDc[ nTable ], p + 1, p + 17, nValues ) )
                return Fail( "has a Huffman table that isn't a prefix code" );
            p += 17 + nValues;
            nLength -= 17 + nValues;
        }
        return TRUE;
    }

    BOOL CDecoder::ReadFrame( const BYTE *p, int nLength )
    {
        if ( m_bFrame )
            return Fail( "has more than one frame" );
        if ( nLength < 6 )
            return Fail( "has a damaged frame header" );
        if ( p[ 0 ] != 8 )
            return Fail( "has " + std::to_string( p[ 0 ] ) + "-bit samples; Ray reads 8-bit JPEGs" );
        m_nHeight = Big16( p + 1 );
        m_nWidth = Big16( p + 3 );

        int nComponents = p[ 5 ];

        if ( m_nHeight == 0 )
            return Fail( "gives its height after the image data (a DNL marker), which Ray doesn't read" );
        if ( m_nWidth == 0 )
            return Fail( "has no width" );
        if ( (int64_t)m_nWidth * m_nHeight > kMaxPixels )
            return Fail( "is too large (Ray reads JPEGs of up to 64 million pixels)" );
        if ( nComponents == 4 )
            return Fail( "is a four-component (CMYK or YCCK) JPEG, which Ray doesn't convert" );
        if ( nComponents != 1 && nComponents != 3 )
            return Fail( "has " + std::to_string( nComponents ) + " components; Ray reads grayscale and color JPEGs" );
        if ( nLength < 6 + nComponents * 3 )
            return Fail( "has a damaged frame header" );
        m_aoComponents.resize( (size_t)nComponents );
        for ( int nIndex = 0; nIndex < nComponents; nIndex++ )
        {
            SComponent &oComponent = m_aoComponents[ (size_t)nIndex ];
            const BYTE *q = p + 6 + nIndex * 3;

            oComponent.nId = q[ 0 ];
            oComponent.nH = q[ 1 ] >> 4;
            oComponent.nV = q[ 1 ] & 15;
            oComponent.nQuant = q[ 2 ];
            oComponent.nDcTable = oComponent.nAcTable = 0;
            oComponent.nPredictor = 0;
            if ( oComponent.nH < 1 || oComponent.nH > 4 || oComponent.nV < 1 || oComponent.nV > 4 || oComponent.nQuant > 3 )
                return Fail( "has a damaged frame header" );
            if ( oComponent.nH > m_nMaxH )
                m_nMaxH = oComponent.nH;
            if ( oComponent.nV > m_nMaxV )
                m_nMaxV = oComponent.nV;
        }
        m_nMcusWide = ( m_nWidth + 8 * m_nMaxH - 1 ) / ( 8 * m_nMaxH );
        m_nMcusHigh = ( m_nHeight + 8 * m_nMaxV - 1 ) / ( 8 * m_nMaxV );
        for ( size_t nIndex = 0; nIndex < m_aoComponents.size(); nIndex++ )
        {
            SComponent &oComponent = m_aoComponents[ nIndex ];

            if ( m_nMaxH % oComponent.nH != 0 || m_nMaxV % oComponent.nV != 0 )
                return Fail( "has a chroma subsampling Ray doesn't read" );
            oComponent.nWidth = ( m_nWidth * oComponent.nH + m_nMaxH - 1 ) / m_nMaxH;
            oComponent.nHeight = ( m_nHeight * oComponent.nV + m_nMaxV - 1 ) / m_nMaxV;
            oComponent.nBlocksWide = m_nMcusWide * oComponent.nH;
            oComponent.nBlocksHigh = m_nMcusHigh * oComponent.nV;
            oComponent.anCoefficients.assign( (size_t)oComponent.nBlocksWide * (size_t)oComponent.nBlocksHigh * 64, 0 );
        }
        m_bFrame = TRUE;
        return TRUE;
    }

        // The Exif orientation, from IFD0 of the TIFF data in APP1.
    void CDecoder::ReadExif( const BYTE *p, int nLength )
    {
        if ( nLength < 14 || memcmp( p, "Exif\0\0", 6 ) != 0 )
            return;

        const BYTE *t = p + 6;
        int         nTiff = nLength - 6;
        BOOL        bLittle;

        if ( t[ 0 ] == 'I' && t[ 1 ] == 'I' )
            bLittle = TRUE;
        else if ( t[ 0 ] == 'M' && t[ 1 ] == 'M' )
            bLittle = FALSE;
        else
            return;

        auto Read16 = [&]( int nAt ) { return bLittle ? ( t[ nAt ] | ( t[ nAt + 1 ] << 8 ) ) : ( ( t[ nAt ] << 8 ) | t[ nAt + 1 ] ); };
        auto Read32 = [&]( int nAt ) -> int64_t
        {
            return bLittle ? ( (int64_t)t[ nAt ] | ( (int64_t)t[ nAt + 1 ] << 8 ) | ( (int64_t)t[ nAt + 2 ] << 16 ) | ( (int64_t)t[ nAt + 3 ] << 24 ) )
                           : ( ( (int64_t)t[ nAt ] << 24 ) | ( (int64_t)t[ nAt + 1 ] << 16 ) | ( (int64_t)t[ nAt + 2 ] << 8 ) | (int64_t)t[ nAt + 3 ] );
        };

        if ( Read16( 2 ) != 42 )
            return;

        int64_t nIfd = Read32( 4 );

        if ( nIfd < 8 || nIfd + 2 > nTiff )
            return;

        int nEntries = Read16( (int)nIfd );

        for ( int nEntry = 0; nEntry < nEntries; nEntry++ )
        {
            int64_t nAt = nIfd + 2 + nEntry * 12;

            if ( nAt + 12 > nTiff )
                return;
            if ( Read16( (int)nAt ) == 0x0112 && Read16( (int)nAt + 2 ) == 3 )
            {
                int nValue = Read16( (int)nAt + 8 );

                if ( nValue >= 1 && nValue <= 8 )
                    m_nOrientation = nValue;
                return;
            }
        }
    }

    BOOL CDecoder::Restart( void )
    {
            // Past the bits left in the reader, at the RSTn marker.
        const BYTE *p = m_oBits.p;

        while ( p + 1 < m_oBits.pEnd && ! ( p[ 0 ] == 0xFF && p[ 1 ] >= 0xD0 && p[ 1 ] <= 0xD7 ) )
            p++;
        if ( p + 1 >= m_oBits.pEnd )
            return Fail( "is missing a restart marker" );
        m_oBits.Reset( p + 2, m_oBits.pEnd );
        for ( size_t nIndex = 0; nIndex < m_aoComponents.size(); nIndex++ )
            m_aoComponents[ nIndex ].nPredictor = 0;
        m_nEobRun = 0;
        return TRUE;
    }

        // One block's share of a scan: all of it (baseline), or the
        // spectral band nStart-nEnd at the bits nLow up (progressive, with
        // nHigh the previous scan's low bit, 0 on a first pass).
    BOOL CDecoder::DecodeBlock( SComponent &oComponent, short *pBlock, int nStart, int nEnd, int nHigh, int nLow )
    {
        SBits &b = m_oBits;

        if ( ! m_bProgressive )
        {
            int nSize = b.Decode( m_aoDc[ oComponent.nDcTable ] );

            if ( nSize < 0 || nSize > 11 )
                return Fail( "has damaged image data" );
            oComponent.nPredictor += b.Extend( nSize );
            pBlock[ 0 ] = (short)oComponent.nPredictor;
            for ( int k = 1; k < 64; )
            {
                int rs = b.Decode( m_aoAc[ oComponent.nAcTable ] );

                if ( rs < 0 )
                    return Fail( "has damaged image data" );

                int r = rs >> 4, s = rs & 15;

                if ( s == 0 )
                {
                    if ( r != 15 )
                        break;
                    k += 16;
                    continue;
                }
                k += r;
                if ( k > 63 )
                    return Fail( "has damaged image data" );
                pBlock[ kZigzag[ k ] ] = (short)b.Extend( s );
                k++;
            }
            return TRUE;
        }
        if ( nStart == 0 )
        {
            if ( nHigh == 0 )
            {
                int nSize = b.Decode( m_aoDc[ oComponent.nDcTable ] );

                if ( nSize < 0 || nSize > 11 )
                    return Fail( "has damaged image data" );
                oComponent.nPredictor += b.Extend( nSize );
                pBlock[ 0 ] = (short)( oComponent.nPredictor * ( 1 << nLow ) );
            }
            else if ( b.Bit() )
                pBlock[ 0 ] = (short)( pBlock[ 0 ] | ( 1 << nLow ) );
            return TRUE;
        }
        if ( nHigh == 0 )
        {
            if ( m_nEobRun > 0 )
            {
                m_nEobRun--;
                return TRUE;
            }
            for ( int k = nStart; k <= nEnd; )
            {
                int rs = b.Decode( m_aoAc[ oComponent.nAcTable ] );

                if ( rs < 0 )
                    return Fail( "has damaged image data" );

                int r = rs >> 4, s = rs & 15;

                if ( s == 0 )
                {
                    if ( r < 15 )
                    {
                        m_nEobRun = ( 1 << r ) - 1;
                        if ( r )
                            m_nEobRun += b.Get( r );
                        break;
                    }
                    k += 16;
                    continue;
                }
                k += r;
                if ( k > 63 )
                    return Fail( "has damaged image data" );
                pBlock[ kZigzag[ k ] ] = (short)( b.Extend( s ) * ( 1 << nLow ) );
                k++;
            }
            return TRUE;
        }

            // Refining AC coefficients already seen, and placing new ones.
        short nBit = (short)( 1 << nLow );
        int   k = nStart;

        if ( m_nEobRun <= 0 )
        {
            while ( k <= nEnd )
            {
                int rs = b.Decode( m_aoAc[ oComponent.nAcTable ] );

                if ( rs < 0 )
                    return Fail( "has damaged image data" );

                int   r = rs >> 4, s = rs & 15;
                short nNew = 0;

                if ( s == 0 )
                {
                    if ( r < 15 )
                    {
                        m_nEobRun = 1 << r;
                        if ( r )
                            m_nEobRun += b.Get( r );
                        break;      // the rest of the block is refined below
                    }
                }
                else
                {
                    if ( s != 1 )
                        return Fail( "has damaged image data" );
                    nNew = b.Bit() ? nBit : (short)-nBit;
                }
                while ( k <= nEnd )
                {
                    short *p = &pBlock[ kZigzag[ k++ ] ];

                    if ( *p != 0 )
                    {
                        if ( b.Bit() && ( *p & nBit ) == 0 )
                            *p = (short)( *p >= 0 ? *p + nBit : *p - nBit );
                    }
                    else
                    {
                        if ( r == 0 )
                        {
                            if ( nNew )
                                *p = nNew;
                            break;
                        }
                        r--;
                    }
                }
            }
        }
        if ( m_nEobRun > 0 )
        {
            for ( ; k <= nEnd; k++ )
            {
                short *p = &pBlock[ kZigzag[ k ] ];

                if ( *p != 0 && b.Bit() && ( *p & nBit ) == 0 )
                    *p = (short)( *p >= 0 ? *p + nBit : *p - nBit );
            }
            m_nEobRun--;
        }
        return TRUE;
    }

    BOOL CDecoder::ReadScan( const BYTE *p, int nLength, size_t &nEnd )
    {
        if ( ! m_bFrame )
            return Fail( "has image data before its frame header" );
        if ( nLength < 1 )
            return Fail( "has a damaged scan header" );

        int nCount = p[ 0 ];

        if ( nCount < 1 || nCount > 4 || nLength < 4 + nCount * 2 )
            return Fail( "has a damaged scan header" );

        std::vector<SComponent *> apScan;

        for ( int nIndex = 0; nIndex < nCount; nIndex++ )
        {
            int         nId = p[ 1 + nIndex * 2 ], nTables = p[ 2 + nIndex * 2 ];
            SComponent *pFound = NULL;

            for ( size_t nAt = 0; nAt < m_aoComponents.size(); nAt++ )
                if ( m_aoComponents[ nAt ].nId == nId )
                    pFound = &m_aoComponents[ nAt ];
            if ( pFound == NULL || ( nTables >> 4 ) > 3 || ( nTables & 15 ) > 3 )
                return Fail( "has a damaged scan header" );
            pFound->nDcTable = nTables >> 4;
            pFound->nAcTable = nTables & 15;
            apScan.push_back( pFound );
        }

        const BYTE *q = p + 1 + nCount * 2;
        int         nStart = q[ 0 ], nStop = q[ 1 ], nHigh = q[ 2 ] >> 4, nLow = q[ 2 ] & 15;

        if ( m_bProgressive )
        {
            if ( nStart > 63 || nStop > 63 || nStart > nStop || nLow > 13 || nHigh > 13 ||
                 ( nStart == 0 && nStop != 0 ) || ( nStart > 0 && nCount != 1 ) )
                return Fail( "has a damaged progressive scan" );
        }
        else if ( nStart != 0 || nStop != 63 || nHigh != 0 || nLow != 0 )
            return Fail( "has a damaged scan header" );
        for ( size_t nIndex = 0; nIndex < apScan.size(); nIndex++ )
        {
            BOOL bDc = ( nStart == 0 && nHigh == 0 );
            BOOL bAc = ( ! m_bProgressive || nStart > 0 );

            if ( ( bDc && ! m_aoDc[ apScan[ nIndex ]->nDcTable ].bDefined ) || ( bAc && ! m_aoAc[ apScan[ nIndex ]->nAcTable ].bDefined ) )
                return Fail( "uses a Huffman table it doesn't define" );
            apScan[ nIndex ]->nPredictor = 0;
        }
        m_nEobRun = 0;
        m_oBits.Reset( p + nLength, m_pData + m_nSize );

        int nUnit = 0;      // MCUs (or blocks) since the last restart

        if ( nCount == 1 )
        {
            SComponent &oComponent = *apScan[ 0 ];
            int         nWide = ( oComponent.nWidth + 7 ) / 8, nHigh8 = ( oComponent.nHeight + 7 ) / 8;

            for ( int y = 0; y < nHigh8; y++ )
                for ( int x = 0; x < nWide; x++ )
                {
                    if ( m_nRestart && nUnit == m_nRestart )
                    {
                        if ( ! Restart() )
                            return FALSE;
                        nUnit = 0;
                    }
                    if ( ! DecodeBlock( oComponent, oComponent.Block( x, y ), nStart, nStop, nHigh, nLow ) )
                        return FALSE;
                    if ( m_oBits.CutOff() )
                        return Fail( "is cut off" );
                    nUnit++;
                }
        }
        else
        {
            for ( int my = 0; my < m_nMcusHigh; my++ )
                for ( int mx = 0; mx < m_nMcusWide; mx++ )
                {
                    if ( m_nRestart && nUnit == m_nRestart )
                    {
                        if ( ! Restart() )
                            return FALSE;
                        nUnit = 0;
                    }
                    for ( size_t nIndex = 0; nIndex < apScan.size(); nIndex++ )
                    {
                        SComponent &oComponent = *apScan[ nIndex ];

                        for ( int v = 0; v < oComponent.nV; v++ )
                            for ( int h = 0; h < oComponent.nH; h++ )
                                if ( ! DecodeBlock( oComponent, oComponent.Block( mx * oComponent.nH + h, my * oComponent.nV + v ),
                                                    nStart, nStop, nHigh, nLow ) )
                                    return FALSE;
                    }
                    if ( m_oBits.CutOff() )
                        return Fail( "is cut off" );
                    nUnit++;
                }
        }

            // Past the scan's data, to the next marker that isn't a restart.
        const BYTE *r = m_oBits.p;
        const BYTE *pEnd = m_pData + m_nSize;

        while ( r + 1 < pEnd && ! ( r[ 0 ] == 0xFF && r[ 1 ] != 0 && ! ( r[ 1 ] >= 0xD0 && r[ 1 ] <= 0xD7 ) && r[ 1 ] != 0xFF ) )
            r++;
        nEnd = (size_t)( r - m_pData );
        return TRUE;
    }

        // A component's samples brought up to the full image size.
    void CDecoder::Upsample( const SComponent &oComponent, std::vector<BYTE> &anOut ) const
    {
        int         nScaleX = m_nMaxH / oComponent.nH, nScaleY = m_nMaxV / oComponent.nV;
        int         nStride = oComponent.nBlocksWide * 8;
        int         w = oComponent.nWidth, h = oComponent.nHeight;
        const BYTE *s = oComponent.anSamples.data();

        anOut.resize( (size_t)m_nWidth * (size_t)m_nHeight );
        if ( nScaleX == 1 && nScaleY == 1 )
        {
            for ( int y = 0; y < m_nHeight; y++ )
                memcpy( &anOut[ (size_t)y * m_nWidth ], s + (size_t)y * nStride, (size_t)m_nWidth );
            return;
        }
        if ( nScaleX == 2 && ( nScaleY == 1 || nScaleY == 2 ) )
        {
                // libjpeg's fancy upsampling (jdsample.c): a triangle filter,
                // 3/4 of the nearer sample and 1/4 of the next.
            std::vector<int> anSums( (size_t)w );
            std::vector<BYTE> anRow( (size_t)w * 2 );

            for ( int y = 0; y < m_nHeight; y++ )
            {
                int nSource = y / nScaleY;

                if ( nScaleY == 1 )
                {
                    const BYTE *in = s + (size_t)nSource * nStride;

                    if ( w == 1 )
                        anRow[ 0 ] = anRow[ 1 ] = in[ 0 ];
                    else
                    {
                        anRow[ 0 ] = in[ 0 ];
                        anRow[ 1 ] = (BYTE)( ( in[ 0 ] * 3 + in[ 1 ] + 2 ) >> 2 );
                        for ( int x = 1; x < w - 1; x++ )
                        {
                            anRow[ 2 * x ] = (BYTE)( ( in[ x ] * 3 + in[ x - 1 ] + 1 ) >> 2 );
                            anRow[ 2 * x + 1 ] = (BYTE)( ( in[ x ] * 3 + in[ x + 1 ] + 2 ) >> 2 );
                        }
                        anRow[ 2 * w - 2 ] = (BYTE)( ( in[ w - 1 ] * 3 + in[ w - 2 ] + 1 ) >> 2 );
                        anRow[ 2 * w - 1 ] = in[ w - 1 ];
                    }
                }
                else
                {
                        // The other row is the one above for the upper output
                        // row of a pair, the one below for the lower, with the
                        // image's first and last rows repeated past its edges.
                    int nOther = ( y & 1 ) ? nSource + 1 : nSource - 1;

                    if ( nOther < 0 )
                        nOther = 0;
                    if ( nOther > h - 1 )
                        nOther = h - 1;

                    const BYTE *in = s + (size_t)nSource * nStride;
                    const BYTE *other = s + (size_t)nOther * nStride;

                    for ( int x = 0; x < w; x++ )
                        anSums[ (size_t)x ] = in[ x ] * 3 + other[ x ];
                    if ( w == 1 )
                        anRow[ 0 ] = anRow[ 1 ] = (BYTE)( ( anSums[ 0 ] * 4 + 8 ) >> 4 );
                    else
                    {
                        anRow[ 0 ] = (BYTE)( ( anSums[ 0 ] * 4 + 8 ) >> 4 );
                        anRow[ 1 ] = (BYTE)( ( anSums[ 0 ] * 3 + anSums[ 1 ] + 7 ) >> 4 );
                        for ( int x = 1; x < w - 1; x++ )
                        {
                            anRow[ 2 * x ] = (BYTE)( ( anSums[ (size_t)x ] * 3 + anSums[ (size_t)x - 1 ] + 8 ) >> 4 );
                            anRow[ 2 * x + 1 ] = (BYTE)( ( anSums[ (size_t)x ] * 3 + anSums[ (size_t)x + 1 ] + 7 ) >> 4 );
                        }
                        anRow[ 2 * w - 2 ] = (BYTE)( ( anSums[ (size_t)w - 1 ] * 3 + anSums[ (size_t)w - 2 ] + 8 ) >> 4 );
                        anRow[ 2 * w - 1 ] = (BYTE)( ( anSums[ (size_t)w - 1 ] * 4 + 7 ) >> 4 );
                    }
                }
                memcpy( &anOut[ (size_t)y * m_nWidth ], anRow.data(), (size_t)m_nWidth );
            }
            return;
        }
        for ( int y = 0; y < m_nHeight; y++ )
            for ( int x = 0; x < m_nWidth; x++ )
                anOut[ (size_t)y * m_nWidth + x ] = s[ (size_t)( y / nScaleY ) * nStride + x / nScaleX ];
    }

    BOOL CDecoder::Finish( int &nWidth, int &nHeight, std::vector<BYTE> &anPixels )
    {
        for ( size_t nIndex = 0; nIndex < m_aoComponents.size(); nIndex++ )
        {
            SComponent &oComponent = m_aoComponents[ nIndex ];

            if ( ! m_abQuant[ oComponent.nQuant ] )
                return Fail( "uses a quantization table it doesn't define" );

            int nStride = oComponent.nBlocksWide * 8;

            oComponent.anSamples.assign( (size_t)nStride * (size_t)oComponent.nBlocksHigh * 8, 0 );
            for ( int y = 0; y < oComponent.nBlocksHigh; y++ )
                for ( int x = 0; x < oComponent.nBlocksWide; x++ )
                    InverseDct( oComponent.Block( x, y ), m_aanQuant[ oComponent.nQuant ],
                                &oComponent.anSamples[ (size_t)y * 8 * nStride + (size_t)x * 8 ], nStride );
            std::vector<short>().swap( oComponent.anCoefficients );
        }

        std::vector<BYTE> anPlanes[ 3 ];
        size_t            nPixels = (size_t)m_nWidth * (size_t)m_nHeight;

        for ( size_t nIndex = 0; nIndex < m_aoComponents.size(); nIndex++ )
            Upsample( m_aoComponents[ nIndex ], anPlanes[ nIndex ] );

        std::vector<BYTE> anRgb( nPixels * 3 );

        if ( m_aoComponents.size() == 1 )
        {
            for ( size_t n = 0; n < nPixels; n++ )
                anRgb[ n * 3 ] = anRgb[ n * 3 + 1 ] = anRgb[ n * 3 + 2 ] = anPlanes[ 0 ][ n ];
        }
        else
        {
            BOOL bRgb = ( m_nAdobe == 0 ) ||
                        ( m_nAdobe < 0 && m_aoComponents[ 0 ].nId == 'R' && m_aoComponents[ 1 ].nId == 'G' && m_aoComponents[ 2 ].nId == 'B' );

            for ( size_t n = 0; n < nPixels; n++ )
            {
                int y = anPlanes[ 0 ][ n ], cb = anPlanes[ 1 ][ n ] - 128, cr = anPlanes[ 2 ][ n ] - 128;

                if ( bRgb )
                {
                    anRgb[ n * 3 ] = anPlanes[ 0 ][ n ];
                    anRgb[ n * 3 + 1 ] = anPlanes[ 1 ][ n ];
                    anRgb[ n * 3 + 2 ] = anPlanes[ 2 ][ n ];
                    continue;
                }
                    // libjpeg's fixed-point YCbCr to RGB (jdcolor.c).
                anRgb[ n * 3 ] = Clamp( y + ( ( 91881 * cr + 32768 ) >> 16 ) );
                anRgb[ n * 3 + 1 ] = Clamp( y + ( ( -22554 * cb - 46802 * cr + 32768 ) >> 16 ) );
                anRgb[ n * 3 + 2 ] = Clamp( y + ( ( 116130 * cb + 32768 ) >> 16 ) );
            }
        }

            // Turned upright by the Exif orientation.
        int  w = m_nWidth, h = m_nHeight;
        BOOL bSwap = ( m_nOrientation >= 5 );

        nWidth = bSwap ? h : w;
        nHeight = bSwap ? w : h;
        if ( m_nOrientation == 1 )
        {
            anPixels.swap( anRgb );
            return TRUE;
        }
        anPixels.resize( nPixels * 3 );
        for ( int Y = 0; Y < nHeight; Y++ )
            for ( int X = 0; X < nWidth; X++ )
            {
                int x = X, y = Y;

                switch ( m_nOrientation )
                {
                    case 2: x = w - 1 - X; y = Y; break;
                    case 3: x = w - 1 - X; y = h - 1 - Y; break;
                    case 4: x = X; y = h - 1 - Y; break;
                    case 5: x = Y; y = X; break;
                    case 6: x = Y; y = h - 1 - X; break;
                    case 7: x = w - 1 - Y; y = h - 1 - X; break;
                    case 8: x = w - 1 - Y; y = X; break;
                }
                memcpy( &anPixels[ ( (size_t)Y * nWidth + X ) * 3 ], &anRgb[ ( (size_t)y * w + x ) * 3 ], 3 );
            }
        return TRUE;
    }

    BOOL CDecoder::Run( int &nWidth, int &nHeight, std::vector<BYTE> &anPixels, std::string &strError )
    {
        size_t nAt = 2;
        BOOL   bScans = FALSE;

        for ( ;; )
        {
            while ( nAt < m_nSize && m_pData[ nAt ] != 0xFF )
                nAt++;
            while ( nAt < m_nSize && m_pData[ nAt ] == 0xFF )
                nAt++;
            if ( nAt >= m_nSize )
            {
                Fail( "is cut off (it has no end-of-image marker)" );
                break;
            }

            int nMarker = m_pData[ nAt++ ];

            if ( nMarker == 0xD9 )
                break;
            if ( ( nMarker >= 0xD0 && nMarker <= 0xD7 ) || nMarker == 0x01 )
                continue;
            if ( nAt + 2 > m_nSize )
            {
                Fail( "is cut off" );
                break;
            }

            int nLength = Big16( m_pData + nAt );

            if ( nLength < 2 || nAt + (size_t)nLength > m_nSize )
            {
                Fail( "is cut off" );
                break;
            }

            const BYTE *p = m_pData + nAt + 2;
            int         nBody = nLength - 2;
            BOOL        bOk = TRUE;

            switch ( nMarker )
            {
                case 0xC0:
                case 0xC1:
                    m_bProgressive = FALSE;
                    bOk = ReadFrame( p, nBody );
                    break;
                case 0xC2:
                    m_bProgressive = TRUE;
                    bOk = ReadFrame( p, nBody );
                    break;
                case 0xC3:
                case 0xC7:
                case 0xCB:
                case 0xCF:
                    bOk = Fail( "is a lossless JPEG, which Ray doesn't read" );
                    break;
                case 0xC5:
                case 0xC6:
                    bOk = Fail( "is a hierarchical JPEG, which Ray doesn't read" );
                    break;
                case 0xC9:
                case 0xCA:
                case 0xCD:
                case 0xCE:
                case 0xCC:
                    bOk = Fail( "uses arithmetic coding, which Ray doesn't read (re-save it with Huffman coding)" );
                    break;
                case 0xC4:
                    bOk = ReadHuffman( p, nBody );
                    break;
                case 0xDB:
                    bOk = ReadQuant( p, nBody );
                    break;
                case 0xDD:
                    if ( nBody < 2 )
                        bOk = Fail( "has a damaged restart interval" );
                    else
                        m_nRestart = Big16( p );
                    break;
                case 0xDC:
                    bOk = Fail( "gives its height after the image data (a DNL marker), which Ray doesn't read" );
                    break;
                case 0xE1:
                    ReadExif( p, nBody );
                    break;
                case 0xEE:
                    if ( nBody >= 12 && memcmp( p, "Adobe", 5 ) == 0 )
                        m_nAdobe = p[ 11 ];
                    break;
                case 0xDA:
                {
                    size_t nEnd = 0;

                    bOk = ReadScan( p, nBody, nEnd );
                    if ( bOk )
                    {
                        bScans = TRUE;
                        nAt = nEnd;
                        continue;
                    }
                    break;
                }
                default:
                    break;      // APPn, comments and the like
            }
            if ( ! bOk )
                break;
            nAt += (size_t)nLength;
        }
        if ( m_strError.empty() && ! bScans )
            Fail( "has no image data" );
        if ( ! m_strError.empty() )
        {
            strError = m_strError;
            return FALSE;
        }
        if ( ! Finish( nWidth, nHeight, anPixels ) )
        {
            strError = m_strError;
            return FALSE;
        }
        return TRUE;
    }
}

BOOL CJpegImage::IsJpeg( const std::string &strData )
{
    return strData.size() >= 3 && (BYTE)strData[ 0 ] == 0xFF && (BYTE)strData[ 1 ] == 0xD8 && (BYTE)strData[ 2 ] == 0xFF;
}

BOOL CJpegImage::Decode( const std::string &strData, int &nWidth, int &nHeight, std::vector<BYTE> &anPixels, std::string &strError )
{
    nWidth = nHeight = 0;
    anPixels.clear();
    if ( ! IsJpeg( strData ) )
    {
        strError = "isn't a JPEG";
        return FALSE;
    }

    CDecoder oDecoder( strData );

    return oDecoder.Run( nWidth, nHeight, anPixels, strError );
}
