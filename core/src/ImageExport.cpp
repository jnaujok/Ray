/*************************************************************************
*   CImageExport                                                         *
*                                                                        *
*   PNG and Radiance HDR output. See ImageExport.h.                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#include "Globals.h"

#include "ImageExport.h"

#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

namespace
{
        // Writes bits into bytes, least significant bit first, as deflate
        // wants.
    class CBitWriter
    {
        public:
            std::vector<BYTE>   m_anOut;
            uint32_t            m_nBits = 0;
            int                 m_nCount = 0;

            void Put( uint32_t nValue, int nBits )
            {
                m_nBits |= nValue << m_nCount;
                m_nCount += nBits;
                while ( m_nCount >= 8 )
                {
                    m_anOut.push_back( (BYTE)( m_nBits & 0xFF ) );
                    m_nBits >>= 8;
                    m_nCount -= 8;
                }
            }

                // A Huffman code goes most significant bit first.
            void PutCode( uint32_t nCode, int nBits )
            {
                uint32_t nReversed = 0;

                for ( int nBit = 0; nBit < nBits; nBit++ )
                    nReversed |= ( ( nCode >> nBit ) & 1 ) << ( nBits - 1 - nBit );
                Put( nReversed, nBits );
            }

            void Flush( void )
            {
                if ( m_nCount > 0 )
                    m_anOut.push_back( (BYTE)( m_nBits & 0xFF ) );
                m_nBits = 0;
                m_nCount = 0;
            }
    };

        // The fixed Huffman code for a literal byte or length symbol.
    void PutLiteral( CBitWriter &oBits, int nSymbol )
    {
        if ( nSymbol < 144 )
            oBits.PutCode( 0x30 + nSymbol, 8 );
        else if ( nSymbol < 256 )
            oBits.PutCode( 0x190 + ( nSymbol - 144 ), 9 );
        else if ( nSymbol < 280 )
            oBits.PutCode( nSymbol - 256, 7 );
        else
            oBits.PutCode( 0xC0 + ( nSymbol - 280 ), 8 );
    }

    const int s_anLengthBase[ 29 ] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99,
                                       115, 131, 163, 195, 227, 258 };
    const int s_anLengthExtra[ 29 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
    const int s_anDistanceBase[ 30 ] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025,
                                         1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
    const int s_anDistanceExtra[ 30 ] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
                                          12, 12, 13, 13 };

    void PutMatch( CBitWriter &oBits, int nLength, int nDistance )
    {
        int nCode = 28;

        while ( s_anLengthBase[ nCode ] > nLength )
            nCode--;
        PutLiteral( oBits, 257 + nCode );
        oBits.Put( (uint32_t)( nLength - s_anLengthBase[ nCode ] ), s_anLengthExtra[ nCode ] );

        nCode = 29;
        while ( s_anDistanceBase[ nCode ] > nDistance )
            nCode--;
        oBits.PutCode( (uint32_t)nCode, 5 );
        oBits.Put( (uint32_t)( nDistance - s_anDistanceBase[ nCode ] ), s_anDistanceExtra[ nCode ] );
    }

    uint32_t Crc32( const BYTE *pData, size_t nLength, uint32_t nCrc = 0 )
    {
        static uint32_t s_anTable[ 256 ];
        static bool     s_bMade = false;

        if ( ! s_bMade )
        {
            for ( uint32_t nIndex = 0; nIndex < 256; nIndex++ )
            {
                uint32_t nValue = nIndex;

                for ( int nBit = 0; nBit < 8; nBit++ )
                    nValue = ( nValue & 1 ) ? ( 0xEDB88320u ^ ( nValue >> 1 ) ) : ( nValue >> 1 );
                s_anTable[ nIndex ] = nValue;
            }
            s_bMade = true;
        }
        nCrc = ~nCrc;
        for ( size_t nIndex = 0; nIndex < nLength; nIndex++ )
            nCrc = s_anTable[ ( nCrc ^ pData[ nIndex ] ) & 0xFF ] ^ ( nCrc >> 8 );
        return ~nCrc;
    }

    void PutBigEndian( std::vector<BYTE> &anOut, uint32_t nValue )
    {
        anOut.push_back( (BYTE)( nValue >> 24 ) );
        anOut.push_back( (BYTE)( nValue >> 16 ) );
        anOut.push_back( (BYTE)( nValue >> 8 ) );
        anOut.push_back( (BYTE)nValue );
    }

    void PutChunk( std::vector<BYTE> &anOut, const char *strType, const std::vector<BYTE> &anBody )
    {
        std::vector<BYTE> anTyped( strType, strType + 4 );

        anTyped.insert( anTyped.end(), anBody.begin(), anBody.end() );
        PutBigEndian( anOut, (uint32_t)anBody.size() );
        anOut.insert( anOut.end(), anTyped.begin(), anTyped.end() );
        PutBigEndian( anOut, Crc32( anTyped.data(), anTyped.size() ) );
    }

    BOOL SaveBytes( const std::string &strFile, const std::vector<BYTE> &anData )
    {
        FILE *pFile = fopen( strFile.c_str(), "wb" );

        if ( pFile == NULL )
            return FALSE;

        BOOL bOk = ( fwrite( anData.data(), 1, anData.size(), pFile ) == anData.size() ) ? TRUE : FALSE;

        if ( fclose( pFile ) != 0 )
            bOk = FALSE;
        return bOk;
    }
}

std::vector<BYTE> CImageExport::Deflate( const std::vector<BYTE> &anData )
{
    const int           nWindow = 32768;
    const int           nHashSize = 1 << 15;
    const int           nMaxChain = 64;
    std::vector<int>    anHead( nHashSize, -1 );
    std::vector<int>    anPrevious( anData.size(), -1 );
    CBitWriter          oBits;
    size_t              nSize = anData.size();
    uint32_t            nA = 1, nB = 0;

        // zlib: deflate, a 32K window, no dictionary.
    oBits.m_anOut.push_back( 0x78 );
    oBits.m_anOut.push_back( 0x01 );

        // One final block with the fixed codes.
    oBits.Put( 1, 1 );
    oBits.Put( 1, 2 );

    auto Hash = [ & ]( size_t nAt ) -> int
    {
        return (int)( ( ( (uint32_t)anData[ nAt ] << 10 ) ^ ( (uint32_t)anData[ nAt + 1 ] << 5 ) ^ anData[ nAt + 2 ] ) & ( nHashSize - 1 ) );
    };
    auto Insert = [ & ]( size_t nAt )
    {
        if ( nAt + 2 < nSize )
        {
            int nHash = Hash( nAt );

            anPrevious[ nAt ] = anHead[ (size_t)nHash ];
            anHead[ (size_t)nHash ] = (int)nAt;
        }
    };

    size_t nAt = 0;

    while ( nAt < nSize )
    {
        int nBestLength = 0;
        int nBestDistance = 0;

            // The longest earlier match within the window (greedy).
        if ( nAt + 2 < nSize )
        {
            int nCandidate = anHead[ (size_t)Hash( nAt ) ];
            int nChain = 0;
            int nLimit = (int)(std::min<size_t>)( 258, nSize - nAt );

            while ( nCandidate >= 0 && (int)nAt - nCandidate <= nWindow && nChain++ < nMaxChain )
            {
                int nLength = 0;

                while ( nLength < nLimit && anData[ (size_t)nCandidate + (size_t)nLength ] == anData[ nAt + (size_t)nLength ] )
                    nLength++;
                if ( nLength > nBestLength )
                {
                    nBestLength = nLength;
                    nBestDistance = (int)nAt - nCandidate;
                    if ( nLength == nLimit )
                        break;
                }
                nCandidate = anPrevious[ (size_t)nCandidate ];
            }
        }

        if ( nBestLength >= 3 )
        {
            PutMatch( oBits, nBestLength, nBestDistance );
            for ( int nStep = 0; nStep < nBestLength; nStep++ )
                Insert( nAt + (size_t)nStep );
            nAt += (size_t)nBestLength;
        }
        else
        {
            PutLiteral( oBits, anData[ nAt ] );
            Insert( nAt );
            nAt++;
        }
    }
    PutLiteral( oBits, 256 );      // the end of the block
    oBits.Flush();

    for ( size_t nIndex = 0; nIndex < nSize; nIndex++ )
    {
        nA = ( nA + anData[ nIndex ] ) % 65521;
        nB = ( nB + nA ) % 65521;
    }
    PutBigEndian( oBits.m_anOut, ( nB << 16 ) | nA );
    return oBits.m_anOut;
}

std::vector<BYTE> CImageExport::EncodePng( int nWidth, int nHeight, const std::vector<BYTE> &anPixels )
{
    static const BYTE   s_anSignature[ 8 ] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
    std::vector<BYTE>   anOut( s_anSignature, s_anSignature + 8 );
    std::vector<BYTE>   anHeader;
    std::vector<BYTE>   anRows;
    size_t              nStride = (size_t)nWidth * 3;

    PutBigEndian( anHeader, (uint32_t)nWidth );
    PutBigEndian( anHeader, (uint32_t)nHeight );
    anHeader.push_back( 8 );        // bits a channel
    anHeader.push_back( 2 );        // RGB
    anHeader.push_back( 0 );        // deflate
    anHeader.push_back( 0 );        // the standard filters
    anHeader.push_back( 0 );        // not interlaced
    PutChunk( anOut, "IHDR", anHeader );

        // Each row with the Paeth filter: every byte less its prediction
        // from the pixels to the left, above, and above left.
    anRows.reserve( ( nStride + 1 ) * (size_t)nHeight );
    for ( int nY = 0; nY < nHeight; nY++ )
    {
        const BYTE *pRow = anPixels.data() + ( nStride * (size_t)nY );
        const BYTE *pAbove = ( nY > 0 ) ? pRow - nStride : NULL;

        anRows.push_back( 4 );
        for ( size_t nX = 0; nX < nStride; nX++ )
        {
            int nLeft = ( nX >= 3 ) ? pRow[ nX - 3 ] : 0;
            int nUp = pAbove ? pAbove[ nX ] : 0;
            int nUpLeft = ( pAbove && nX >= 3 ) ? pAbove[ nX - 3 ] : 0;
            int nGuess = nLeft + nUp - nUpLeft;
            int nToLeft = abs( nGuess - nLeft );
            int nToUp = abs( nGuess - nUp );
            int nToUpLeft = abs( nGuess - nUpLeft );
            int nPredict = ( nToLeft <= nToUp && nToLeft <= nToUpLeft ) ? nLeft : ( nToUp <= nToUpLeft ) ? nUp : nUpLeft;

            anRows.push_back( (BYTE)( pRow[ nX ] - nPredict ) );
        }
    }
    PutChunk( anOut, "IDAT", Deflate( anRows ) );
    PutChunk( anOut, "IEND", std::vector<BYTE>() );
    return anOut;
}

BOOL CImageExport::WritePng( const std::string &strFile, int nWidth, int nHeight, const std::vector<BYTE> &anPixels )
{
    if ( nWidth <= 0 || nHeight <= 0 || anPixels.size() != (size_t)nWidth * (size_t)nHeight * 3 )
        return FALSE;
    return SaveBytes( strFile, EncodePng( nWidth, nHeight, anPixels ) );
}

void CImageExport::ToRgbe( float nRed, float nGreen, float nBlue, BYTE anRgbe[ 4 ] )
{
    float anValue[ 3 ] = { nRed, nGreen, nBlue };

    for ( int nIndex = 0; nIndex < 3; nIndex++ )
    {
        if ( ! ( anValue[ nIndex ] > 0.0f ) || ! isfinite( anValue[ nIndex ] ) )
            anValue[ nIndex ] = ( anValue[ nIndex ] > 0.0f ) ? 3.0e38f : 0.0f;   // +inf saturates
    }

    float nLargest = (std::max)( anValue[ 0 ], (std::max)( anValue[ 1 ], anValue[ 2 ] ) );

    if ( nLargest < 1e-32f )
    {
        anRgbe[ 0 ] = anRgbe[ 1 ] = anRgbe[ 2 ] = anRgbe[ 3 ] = 0;
        return;
    }

    int     nExponent;
    float   nScale = (float)( frexp( (double)nLargest, &nExponent ) * 256.0 / (double)nLargest );

    for ( int nIndex = 0; nIndex < 3; nIndex++ )
    {
        float nByte = anValue[ nIndex ] * nScale;

        anRgbe[ nIndex ] = (BYTE)( ( nByte >= 255.0f ) ? 255 : (int)nByte );
    }
    anRgbe[ 3 ] = (BYTE)( nExponent + 128 );
}

BOOL CImageExport::WriteHdr( const std::string &strFile, int nWidth, int nHeight, const std::vector<float> &anPixels )
{
    if ( nWidth <= 0 || nHeight <= 0 || anPixels.size() != (size_t)nWidth * (size_t)nHeight * 3 )
        return FALSE;

    std::string         strHeader = "#?RADIANCE\n# Written by Ray\nFORMAT=32-bit_rle_rgbe\n\n-Y " +
                                    std::to_string( nHeight ) + " +X " + std::to_string( nWidth ) + "\n";
    std::vector<BYTE>   anOut( strHeader.begin(), strHeader.end() );

        // Flat (not run-length encoded) scanlines, top to bottom.
    anOut.reserve( anOut.size() + ( (size_t)nWidth * (size_t)nHeight * 4 ) );
    for ( size_t nPixel = 0; nPixel < (size_t)nWidth * (size_t)nHeight; nPixel++ )
    {
        BYTE anRgbe[ 4 ];

        ToRgbe( anPixels[ nPixel * 3 ], anPixels[ ( nPixel * 3 ) + 1 ], anPixels[ ( nPixel * 3 ) + 2 ], anRgbe );
        anOut.insert( anOut.end(), anRgbe, anRgbe + 4 );
    }
    return SaveBytes( strFile, anOut );
}

BOOL CImageExport::ReadRaw( const std::string &strFile, int &nWidth, int &nHeight, std::vector<BYTE> &anPixels )
{
    FILE   *pFile = fopen( strFile.c_str(), "rb" );
    BYTE    anHeader[ 4 ];
    BOOL    bOk = FALSE;

    if ( pFile == NULL )
        return FALSE;
    if ( fread( anHeader, 1, 4, pFile ) == 4 )
    {
        nWidth = ( anHeader[ 0 ] * 256 ) + anHeader[ 1 ];
        nHeight = ( anHeader[ 2 ] * 256 ) + anHeader[ 3 ];
        anPixels.resize( (size_t)nWidth * (size_t)nHeight * 3 );
        bOk = ( fread( anPixels.data(), 1, anPixels.size(), pFile ) == anPixels.size() ) ? TRUE : FALSE;
    }
    fclose( pFile );
    return bOk;
}
