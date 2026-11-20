/*************************************************************************
*   CPngImage                                                            *
*                                                                        *
*   Reads PNG images. See PngImage.h.                                    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "PngImage.h"

#include <stdint.h>
#include <string.h>

namespace
{
        // The largest image read: 16384 pixels a side and 2^28 pixels in all,
        // so a damaged size can't ask for gigabytes.
    const uint32_t  MAX_SIDE = 16384;
    const uint64_t  MAX_PIXELS = (uint64_t)1 << 28;

    // ----------------------------------------------------------------------
    // Inflate (RFC 1951), after Mark Adler's puff: canonical Huffman codes
    // decoded a bit at a time, every read checked against the end.
    // ----------------------------------------------------------------------

    struct SBits
    {
        const BYTE *pData;
        size_t      nSize;
        size_t      nAt;
        uint32_t    nBuffer;
        int         nCount;
        BOOL        bShort;     // read past the end

        uint32_t    Get( int nBits )
                    {
                        while ( nCount < nBits )
                        {
                            if ( nAt >= nSize )
                            {
                                bShort = TRUE;
                                return 0;
                            }
                            nBuffer |= (uint32_t)pData[ nAt++ ] << nCount;
                            nCount += 8;
                        }

                        uint32_t nValue = nBuffer & ( ( 1u << nBits ) - 1u );

                        nBuffer >>= nBits;
                        nCount -= nBits;
                        return nValue;
                    }
                    // Drops the bits left in the current byte (a stored block
                    // and the zlib trailer start on a byte boundary).
        void        Align( void ) { nBuffer = 0; nCount = 0; }
    };

    const int MAX_BITS = 15;

    struct SHuffman
    {
        short   anCount[ MAX_BITS + 1 ];    // codes of each length
        short   anSymbol[ 320 ];            // the symbols, by code
    };

        // Builds a code from its symbols' lengths. FALSE if the lengths ask
        // for more codes than there are (an incomplete code is allowed: a
        // code that isn't there fails when it is decoded).
    BOOL Build( SHuffman &oCode, const short *anLength, int nSymbols )
    {
        short   anOffset[ MAX_BITS + 1 ];
        int     nLeft = 1;

        memset( oCode.anCount, 0, sizeof( oCode.anCount ) );
        for ( int nSymbol = 0; nSymbol < nSymbols; nSymbol++ )
            oCode.anCount[ anLength[ nSymbol ] ]++;
        for ( int nLength = 1; nLength <= MAX_BITS; nLength++ )
        {
            nLeft <<= 1;
            nLeft -= oCode.anCount[ nLength ];
            if ( nLeft < 0 )
                return FALSE;
        }
        anOffset[ 1 ] = 0;
        for ( int nLength = 1; nLength < MAX_BITS; nLength++ )
            anOffset[ nLength + 1 ] = (short)( anOffset[ nLength ] + oCode.anCount[ nLength ] );
        for ( int nSymbol = 0; nSymbol < nSymbols; nSymbol++ )
        {
            if ( anLength[ nSymbol ] != 0 )
                oCode.anSymbol[ anOffset[ anLength[ nSymbol ] ]++ ] = (short)nSymbol;
        }
        return TRUE;
    }

        // The next symbol in a code, or -1 if the bits aren't a code in it.
    int Decode( SBits &oBits, const SHuffman &oCode )
    {
        int nCode = 0;
        int nFirst = 0;
        int nIndex = 0;

        for ( int nLength = 1; nLength <= MAX_BITS; nLength++ )
        {
            nCode |= (int)oBits.Get( 1 );
            if ( oBits.bShort )
                return -1;

            int nCount = oCode.anCount[ nLength ];

            if ( nCode - nCount < nFirst )
                return oCode.anSymbol[ nIndex + ( nCode - nFirst ) ];
            nIndex += nCount;
            nFirst += nCount;
            nFirst <<= 1;
            nCode <<= 1;
        }
        return -1;
    }

    const short s_anLengthBase[ 29 ] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
                                         67, 83, 99, 115, 131, 163, 195, 227, 258 };
    const short s_anLengthExtra[ 29 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
    const short s_anDistanceBase[ 30 ] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769,
                                           1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
    const short s_anDistanceExtra[ 30 ] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10,
                                            11, 11, 12, 12, 13, 13 };

        // The symbols of one compressed block, until its end.
    BOOL Codes( SBits &oBits, const SHuffman &oLengths, const SHuffman &oDistances, std::vector<BYTE> &anOut,
                size_t nLimit, std::string &strError )
    {
        for ( ;; )
        {
            int nSymbol = Decode( oBits, oLengths );

            if ( nSymbol < 0 )
            {
                strError = oBits.bShort ? "the compressed data ends early" : "a bad code in the compressed data";
                return FALSE;
            }
            if ( nSymbol < 256 )
            {
                if ( anOut.size() >= nLimit )
                {
                    strError = "the compressed data holds more than the image needs";
                    return FALSE;
                }
                anOut.push_back( (BYTE)nSymbol );
                continue;
            }
            if ( nSymbol == 256 )
                return TRUE;

            nSymbol -= 257;
            if ( nSymbol >= 29 )
            {
                strError = "a bad length code in the compressed data";
                return FALSE;
            }

            size_t  nLength = (size_t)s_anLengthBase[ nSymbol ] + oBits.Get( s_anLengthExtra[ nSymbol ] );
            int     nDistanceSymbol = Decode( oBits, oDistances );

            if ( nDistanceSymbol < 0 || nDistanceSymbol >= 30 || oBits.bShort )
            {
                strError = oBits.bShort ? "the compressed data ends early" : "a bad distance code in the compressed data";
                return FALSE;
            }

            size_t nDistance = (size_t)s_anDistanceBase[ nDistanceSymbol ] + oBits.Get( s_anDistanceExtra[ nDistanceSymbol ] );

            if ( oBits.bShort )
            {
                strError = "the compressed data ends early";
                return FALSE;
            }
            if ( nDistance > anOut.size() )
            {
                strError = "the compressed data refers back before its start";
                return FALSE;
            }
            if ( anOut.size() + nLength > nLimit )
            {
                strError = "the compressed data holds more than the image needs";
                return FALSE;
            }

            size_t nFrom = anOut.size() - nDistance;

            for ( size_t nIndex = 0; nIndex < nLength; nIndex++ )
                anOut.push_back( anOut[ nFrom + nIndex ] );
        }
    }

    struct SFixedCodes
    {
        SHuffman    oLengths;
        SHuffman    oDistances;

                    SFixedCodes( void )
                    {
                        short anLength[ 288 ];
                        int   nSymbol;

                        for ( nSymbol = 0; nSymbol < 144; nSymbol++ )
                            anLength[ nSymbol ] = 8;
                        for ( ; nSymbol < 256; nSymbol++ )
                            anLength[ nSymbol ] = 9;
                        for ( ; nSymbol < 280; nSymbol++ )
                            anLength[ nSymbol ] = 7;
                        for ( ; nSymbol < 288; nSymbol++ )
                            anLength[ nSymbol ] = 8;
                        Build( oLengths, anLength, 288 );
                        for ( nSymbol = 0; nSymbol < 30; nSymbol++ )
                            anLength[ nSymbol ] = 5;
                        Build( oDistances, anLength, 30 );
                    }
    };

    BOOL FixedBlock( SBits &oBits, std::vector<BYTE> &anOut, size_t nLimit, std::string &strError )
    {
        static const SFixedCodes s_oCodes;     // built once, safely, by the first thread here

        return Codes( oBits, s_oCodes.oLengths, s_oCodes.oDistances, anOut, nLimit, strError );
    }

    BOOL DynamicBlock( SBits &oBits, std::vector<BYTE> &anOut, size_t nLimit, std::string &strError )
    {
        static const short s_anOrder[ 19 ] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
        short    anLength[ 320 ];
        SHuffman oLengths;
        SHuffman oDistances;
        int      nLengthCodes = (int)oBits.Get( 5 ) + 257;
        int      nDistanceCodes = (int)oBits.Get( 5 ) + 1;
        int      nCodeCodes = (int)oBits.Get( 4 ) + 4;

        if ( oBits.bShort || nLengthCodes > 286 || nDistanceCodes > 30 )
        {
            strError = oBits.bShort ? "the compressed data ends early" : "a bad block header in the compressed data";
            return FALSE;
        }

        memset( anLength, 0, sizeof( anLength ) );
        for ( int nIndex = 0; nIndex < nCodeCodes; nIndex++ )
            anLength[ s_anOrder[ nIndex ] ] = (short)oBits.Get( 3 );
        if ( oBits.bShort || ! Build( oLengths, anLength, 19 ) )
        {
            strError = oBits.bShort ? "the compressed data ends early" : "a bad code-length code in the compressed data";
            return FALSE;
        }

        int nIndex = 0;

        while ( nIndex < nLengthCodes + nDistanceCodes )
        {
            int nSymbol = Decode( oBits, oLengths );

            if ( nSymbol < 0 )
            {
                strError = oBits.bShort ? "the compressed data ends early" : "a bad code length in the compressed data";
                return FALSE;
            }
            if ( nSymbol < 16 )
            {
                anLength[ nIndex++ ] = (short)nSymbol;
                continue;
            }

            short nRepeat = 0;
            int   nTimes;

            if ( nSymbol == 16 )
            {
                if ( nIndex == 0 )
                {
                    strError = "a repeated code length with nothing before it in the compressed data";
                    return FALSE;
                }
                nRepeat = anLength[ nIndex - 1 ];
                nTimes = 3 + (int)oBits.Get( 2 );
            }
            else if ( nSymbol == 17 )
                nTimes = 3 + (int)oBits.Get( 3 );
            else
                nTimes = 11 + (int)oBits.Get( 7 );
            if ( oBits.bShort || nIndex + nTimes > nLengthCodes + nDistanceCodes )
            {
                strError = oBits.bShort ? "the compressed data ends early" : "too many code lengths in the compressed data";
                return FALSE;
            }
            while ( nTimes-- > 0 )
                anLength[ nIndex++ ] = nRepeat;
        }

        if ( anLength[ 256 ] == 0 )
        {
            strError = "a block with no end code in the compressed data";
            return FALSE;
        }
        if ( ! Build( oLengths, anLength, nLengthCodes ) || ! Build( oDistances, anLength + nLengthCodes, nDistanceCodes ) )
        {
            strError = "a bad Huffman code in the compressed data";
            return FALSE;
        }
        return Codes( oBits, oLengths, oDistances, anOut, nLimit, strError );
    }

    // ----------------------------------------------------------------------
    // PNG
    // ----------------------------------------------------------------------

    uint32_t BigEndian( const BYTE *p )
    {
        return ( (uint32_t)p[ 0 ] << 24 ) | ( (uint32_t)p[ 1 ] << 16 ) | ( (uint32_t)p[ 2 ] << 8 ) | (uint32_t)p[ 3 ];
    }

    struct SCrcTable
    {
        uint32_t    anTable[ 256 ];

                    SCrcTable( void )
                    {
                        for ( uint32_t nIndex = 0; nIndex < 256; nIndex++ )
                        {
                            uint32_t nValue = nIndex;

                            for ( int nBit = 0; nBit < 8; nBit++ )
                                nValue = ( nValue & 1u ) ? ( 0xEDB88320u ^ ( nValue >> 1 ) ) : ( nValue >> 1 );
                            anTable[ nIndex ] = nValue;
                        }
                    }
    };

    uint32_t Crc( const BYTE *pData, size_t nSize )
    {
        static const SCrcTable s_oCrc;
        uint32_t               nCrc = 0xFFFFFFFFu;

        for ( size_t nIndex = 0; nIndex < nSize; nIndex++ )
            nCrc = s_oCrc.anTable[ ( nCrc ^ pData[ nIndex ] ) & 0xFFu ] ^ ( nCrc >> 8 );
        return nCrc ^ 0xFFFFFFFFu;
    }

        // The Adam7 passes: where each starts and how far apart its pixels are.
    const int s_anPassX[ 7 ] = { 0, 4, 0, 2, 0, 1, 0 };
    const int s_anPassY[ 7 ] = { 0, 0, 4, 0, 2, 0, 1 };
    const int s_anPassDX[ 7 ] = { 8, 8, 4, 4, 2, 2, 1 };
    const int s_anPassDY[ 7 ] = { 8, 8, 8, 4, 4, 2, 2 };

    int Paeth( int nLeft, int nUp, int nUpLeft )
    {
        int nGuess = nLeft + nUp - nUpLeft;
        int nToLeft = ( nGuess > nLeft ) ? nGuess - nLeft : nLeft - nGuess;
        int nToUp = ( nGuess > nUp ) ? nGuess - nUp : nUp - nGuess;
        int nToUpLeft = ( nGuess > nUpLeft ) ? nGuess - nUpLeft : nUpLeft - nGuess;

        if ( nToLeft <= nToUp && nToLeft <= nToUpLeft )
            return nLeft;
        return ( nToUp <= nToUpLeft ) ? nUp : nUpLeft;
    }
}

BOOL CPngImage::Inflate( const BYTE *pData, size_t nSize, std::vector<BYTE> &anOut, size_t nLimit, std::string &strError )
{
    anOut.clear();
    if ( nSize < 6 )
    {
        strError = "the compressed data is too short";
        return FALSE;
    }
    if ( ( pData[ 0 ] & 0x0F ) != 8 || ( pData[ 0 ] >> 4 ) > 7 || ( ( pData[ 0 ] << 8 ) | pData[ 1 ] ) % 31 != 0 )
    {
        strError = "the compressed data isn't a zlib stream";
        return FALSE;
    }
    if ( pData[ 1 ] & 0x20 )
    {
        strError = "the compressed data needs a preset dictionary";
        return FALSE;
    }

    SBits oBits = { pData + 2, nSize - 2, 0, 0, 0, FALSE };
    BOOL  bLast = FALSE;

    while ( ! bLast )
    {
        bLast = oBits.Get( 1 ) ? TRUE : FALSE;

        uint32_t nType = oBits.Get( 2 );

        if ( oBits.bShort )
        {
            strError = "the compressed data ends early";
            return FALSE;
        }
        if ( nType == 0 )
        {
            oBits.Align();
            if ( oBits.nAt + 4 > oBits.nSize )
            {
                strError = "the compressed data ends early";
                return FALSE;
            }

            const BYTE *p = oBits.pData + oBits.nAt;
            size_t      nLength = (size_t)p[ 0 ] | ( (size_t)p[ 1 ] << 8 );
            size_t      nCheck = (size_t)p[ 2 ] | ( (size_t)p[ 3 ] << 8 );

            if ( nLength != ( ~nCheck & 0xFFFF ) )
            {
                strError = "a stored block's length is damaged in the compressed data";
                return FALSE;
            }
            oBits.nAt += 4;
            if ( oBits.nAt + nLength > oBits.nSize )
            {
                strError = "the compressed data ends early";
                return FALSE;
            }
            if ( anOut.size() + nLength > nLimit )
            {
                strError = "the compressed data holds more than the image needs";
                return FALSE;
            }
            anOut.insert( anOut.end(), oBits.pData + oBits.nAt, oBits.pData + oBits.nAt + nLength );
            oBits.nAt += nLength;
        }
        else if ( nType == 1 )
        {
            if ( ! FixedBlock( oBits, anOut, nLimit, strError ) )
                return FALSE;
        }
        else if ( nType == 2 )
        {
            if ( ! DynamicBlock( oBits, anOut, nLimit, strError ) )
                return FALSE;
        }
        else
        {
            strError = "a bad block type in the compressed data";
            return FALSE;
        }
    }

        // The Adler-32 checksum of what came out.
    oBits.Align();
    if ( oBits.nAt + 4 > oBits.nSize )
    {
        strError = "the compressed data has no checksum";
        return FALSE;
    }

    uint32_t nA = 1;
    uint32_t nB = 0;

    for ( size_t nIndex = 0; nIndex < anOut.size(); nIndex++ )
    {
        nA = ( nA + anOut[ nIndex ] ) % 65521u;
        nB = ( nB + nA ) % 65521u;
    }
    if ( BigEndian( oBits.pData + oBits.nAt ) != ( ( nB << 16 ) | nA ) )
    {
        strError = "the compressed data's checksum is wrong";
        return FALSE;
    }
    return TRUE;
}

BOOL CPngImage::IsPng( const std::string &strData )
{
    static const BYTE s_anSignature[ 8 ] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };

    return ( strData.size() >= 8 && memcmp( strData.data(), s_anSignature, 8 ) == 0 ) ? TRUE : FALSE;
}

BOOL CPngImage::Decode( const std::string &strData, int &nWidth, int &nHeight, std::vector<BYTE> &anPixels,
                        std::string &strError )
{
    if ( ! IsPng( strData ) )
    {
        strError = "isn't a PNG image";
        return FALSE;
    }

    const BYTE         *pData = (const BYTE *)strData.data();
    size_t              nSize = strData.size();
    size_t              nAt = 8;
    BOOL                bHeader = FALSE;
    BOOL                bEnd = FALSE;
    uint32_t            nImageWidth = 0;
    uint32_t            nImageHeight = 0;
    int                 nDepth = 0;
    int                 nColorType = 0;
    int                 nInterlace = 0;
    std::vector<BYTE>   anPalette;
    std::vector<BYTE>   anCompressed;

    while ( ! bEnd )
    {
        if ( nAt + 8 > nSize )
        {
            strError = "ends before its last chunk";
            return FALSE;
        }

        uint32_t    nLength = BigEndian( pData + nAt );
        const BYTE *pType = pData + nAt + 4;

        if ( nLength > nSize - nAt - 8 || nSize - nAt - 8 - nLength < 4 )
        {
            strError = "has a chunk longer than the file";
            return FALSE;
        }

        const BYTE *pChunk = pData + nAt + 8;

        if ( Crc( pType, (size_t)nLength + 4 ) != BigEndian( pChunk + nLength ) )
        {
            strError = "has a damaged chunk (its CRC is wrong)";
            return FALSE;
        }
        nAt += (size_t)nLength + 12;

        if ( memcmp( pType, "IHDR", 4 ) == 0 )
        {
            if ( bHeader || nLength != 13 )
            {
                strError = "has a bad header";
                return FALSE;
            }
            nImageWidth = BigEndian( pChunk );
            nImageHeight = BigEndian( pChunk + 4 );
            nDepth = pChunk[ 8 ];
            nColorType = pChunk[ 9 ];
            nInterlace = pChunk[ 12 ];

            BOOL bDepthOk = FALSE;

            switch ( nColorType )
            {
                case 0: bDepthOk = ( nDepth == 1 || nDepth == 2 || nDepth == 4 || nDepth == 8 || nDepth == 16 ); break;
                case 3: bDepthOk = ( nDepth == 1 || nDepth == 2 || nDepth == 4 || nDepth == 8 ); break;
                case 2:
                case 4:
                case 6: bDepthOk = ( nDepth == 8 || nDepth == 16 ); break;
                default: break;
            }
            if ( nImageWidth == 0 || nImageHeight == 0 || ! bDepthOk || pChunk[ 10 ] != 0 || pChunk[ 11 ] != 0 || nInterlace > 1 )
            {
                strError = "has a bad header (size, bit depth, color type or method)";
                return FALSE;
            }
            if ( nImageWidth > MAX_SIDE || nImageHeight > MAX_SIDE || (uint64_t)nImageWidth * nImageHeight > MAX_PIXELS )
            {
                strError = "is too large (at most 16384 pixels a side)";
                return FALSE;
            }
            bHeader = TRUE;
            continue;
        }
        if ( ! bHeader )
        {
            strError = "doesn't start with its header";
            return FALSE;
        }
        if ( memcmp( pType, "PLTE", 4 ) == 0 )
        {
            if ( nLength % 3 != 0 || nLength == 0 || nLength > 768 )
            {
                strError = "has a bad palette";
                return FALSE;
            }
            anPalette.assign( pChunk, pChunk + nLength );
        }
        else if ( memcmp( pType, "IDAT", 4 ) == 0 )
            anCompressed.insert( anCompressed.end(), pChunk, pChunk + nLength );
        else if ( memcmp( pType, "IEND", 4 ) == 0 )
            bEnd = TRUE;
        else if ( ( pType[ 0 ] & 0x20 ) == 0 )
        {
                // A critical chunk this reader doesn't know: the image can't
                // be read without it.
            strError = "needs a chunk Ray can't read (\"" + std::string( (const char *)pType, 4 ) + "\")";
            return FALSE;
        }
    }

    if ( nColorType == 3 && anPalette.empty() )
    {
        strError = "is a palette image without a palette";
        return FALSE;
    }
    if ( anCompressed.empty() )
    {
        strError = "has no image data";
        return FALSE;
    }

    static const int s_anChannels[ 7 ] = { 1, 0, 3, 1, 2, 0, 4 };
    int              nChannels = s_anChannels[ nColorType ];
    size_t           nPixelBits = (size_t)nChannels * (size_t)nDepth;
    size_t           nStep = ( nPixelBits + 7 ) / 8;    // bytes a pixel, at least 1, for the filters
    int              nPasses = ( nInterlace == 1 ) ? 7 : 1;
    size_t           anPassWidth[ 7 ];
    size_t           anPassHeight[ 7 ];
    size_t           nExpected = 0;

    for ( int nPass = 0; nPass < nPasses; nPass++ )
    {
        int nX0 = ( nInterlace == 1 ) ? s_anPassX[ nPass ] : 0;
        int nY0 = ( nInterlace == 1 ) ? s_anPassY[ nPass ] : 0;
        int nDX = ( nInterlace == 1 ) ? s_anPassDX[ nPass ] : 1;
        int nDY = ( nInterlace == 1 ) ? s_anPassDY[ nPass ] : 1;

        anPassWidth[ nPass ] = ( nImageWidth > (uint32_t)nX0 ) ? ( nImageWidth - (uint32_t)nX0 + (uint32_t)nDX - 1 ) / (uint32_t)nDX : 0;
        anPassHeight[ nPass ] = ( nImageHeight > (uint32_t)nY0 ) ? ( nImageHeight - (uint32_t)nY0 + (uint32_t)nDY - 1 ) / (uint32_t)nDY : 0;
        if ( anPassWidth[ nPass ] > 0 && anPassHeight[ nPass ] > 0 )
            nExpected += ( 1 + ( ( anPassWidth[ nPass ] * nPixelBits + 7 ) / 8 ) ) * anPassHeight[ nPass ];
    }

    std::vector<BYTE> anRaw;

    if ( ! Inflate( anCompressed.data(), anCompressed.size(), anRaw, nExpected, strError ) )
        return FALSE;
    if ( anRaw.size() != nExpected )
    {
        strError = "has less image data than its size needs";
        return FALSE;
    }

    nWidth = (int)nImageWidth;
    nHeight = (int)nImageHeight;
    anPixels.assign( (size_t)nImageWidth * (size_t)nImageHeight * 3, 0 );

    size_t              nOffset = 0;
    std::vector<BYTE>   anPrevious;
    unsigned            nMaximum = ( 1u << nDepth ) - 1u;
    size_t              nColors = anPalette.size() / 3;

    for ( int nPass = 0; nPass < nPasses; nPass++ )
    {
        size_t nPassWidth = anPassWidth[ nPass ];
        size_t nPassHeight = anPassHeight[ nPass ];

        if ( nPassWidth == 0 || nPassHeight == 0 )
            continue;

        size_t nRowBytes = ( nPassWidth * nPixelBits + 7 ) / 8;
        int    nX0 = ( nInterlace == 1 ) ? s_anPassX[ nPass ] : 0;
        int    nY0 = ( nInterlace == 1 ) ? s_anPassY[ nPass ] : 0;
        int    nDX = ( nInterlace == 1 ) ? s_anPassDX[ nPass ] : 1;
        int    nDY = ( nInterlace == 1 ) ? s_anPassDY[ nPass ] : 1;

        anPrevious.assign( nRowBytes, 0 );
        for ( size_t nRow = 0; nRow < nPassHeight; nRow++ )
        {
            int   nFilter = anRaw[ nOffset ];
            BYTE *pRow = anRaw.data() + nOffset + 1;

            nOffset += nRowBytes + 1;
            for ( size_t nIndex = 0; nIndex < nRowBytes; nIndex++ )
            {
                int nLeft = ( nIndex >= nStep ) ? pRow[ nIndex - nStep ] : 0;
                int nUp = anPrevious[ nIndex ];
                int nUpLeft = ( nIndex >= nStep ) ? anPrevious[ nIndex - nStep ] : 0;

                switch ( nFilter )
                {
                    case 0: break;
                    case 1: pRow[ nIndex ] = (BYTE)( pRow[ nIndex ] + nLeft ); break;
                    case 2: pRow[ nIndex ] = (BYTE)( pRow[ nIndex ] + nUp ); break;
                    case 3: pRow[ nIndex ] = (BYTE)( pRow[ nIndex ] + ( ( nLeft + nUp ) >> 1 ) ); break;
                    case 4: pRow[ nIndex ] = (BYTE)( pRow[ nIndex ] + Paeth( nLeft, nUp, nUpLeft ) ); break;
                    default:
                        strError = "has a row with a bad filter type";
                        return FALSE;
                }
            }

            size_t nY = (size_t)nY0 + nRow * (size_t)nDY;

            for ( size_t nColumn = 0; nColumn < nPassWidth; nColumn++ )
            {
                size_t   nX = (size_t)nX0 + nColumn * (size_t)nDX;
                unsigned anSample[ 4 ] = { 0, 0, 0, 0 };

                for ( int nChannel = 0; nChannel < nChannels; nChannel++ )
                {
                    size_t nSample = nColumn * (size_t)nChannels + (size_t)nChannel;

                    if ( nDepth == 8 )
                        anSample[ nChannel ] = pRow[ nSample ];
                    else if ( nDepth == 16 )
                        anSample[ nChannel ] = ( (unsigned)pRow[ nSample * 2 ] << 8 ) | pRow[ nSample * 2 + 1 ];
                    else
                    {
                        size_t nBit = nSample * (size_t)nDepth;

                        anSample[ nChannel ] = ( pRow[ nBit / 8 ] >> ( 8 - nDepth - (int)( nBit % 8 ) ) ) & nMaximum;
                    }
                }

                BYTE *pOut = &anPixels[ ( nY * nImageWidth + nX ) * 3 ];

                if ( nColorType == 3 )
                {
                    if ( anSample[ 0 ] >= nColors )
                    {
                        strError = "has a pixel whose palette index is past the end of its palette";
                        return FALSE;
                    }
                    memcpy( pOut, &anPalette[ anSample[ 0 ] * 3 ], 3 );
                    continue;
                }

                    // Each channel to 8 bits: scaled up from a low depth so its
                    // largest value is 255, or rounded down from 16.
                BYTE anByte[ 3 ];
                int  nColorChannels = ( nColorType == 2 || nColorType == 6 ) ? 3 : 1;

                for ( int nChannel = 0; nChannel < nColorChannels; nChannel++ )
                {
                    unsigned nValue = anSample[ nChannel ];

                    if ( nDepth == 16 )
                        anByte[ nChannel ] = (BYTE)( ( nValue * 255u + 32767u ) / 65535u );
                    else if ( nDepth < 8 )
                        anByte[ nChannel ] = (BYTE)( ( nValue * 255u ) / nMaximum );
                    else
                        anByte[ nChannel ] = (BYTE)nValue;
                }
                if ( nColorChannels == 1 )
                    anByte[ 1 ] = anByte[ 2 ] = anByte[ 0 ];
                memcpy( pOut, anByte, 3 );
            }
            anPrevious.assign( pRow, pRow + nRowBytes );
        }
    }
    return TRUE;
}
