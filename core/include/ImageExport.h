/*************************************************************************
*   CImageExport                                                         *
*                                                                        *
*   Writes finished images in formats other programs read:               *
*                                                                        *
*   - PNG: 8-bit RGB, the bytes the color model made. Compressed with a  *
*     small deflate encoder of our own (fixed Huffman codes), so no zlib *
*     is needed.                                                         *
*   - Radiance HDR (.hdr, RGBE): the linear colors, unclipped, as        *
*     floating point with a shared exponent: high dynamic range.         *
*                                                                        *
*   Both take pixels row by row from the top left, red, green, blue.     *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __IMAGEEXPORT_H__
#define __IMAGEEXPORT_H__

#include "Globals.h"

#include <string>
#include <vector>

class CImageExport
{
    public:
            /// Reads a Ray raw image (2-byte width and height, big-endian,
            /// then RGB bytes). FALSE if it can't be read or is short.
        static BOOL     ReadRaw( const std::string &strFile, int &nWidth, int &nHeight, std::vector<BYTE> &anPixels );
            /// Writes 8-bit RGB pixels as a PNG.
        static BOOL     WritePng( const std::string &strFile, int nWidth, int nHeight, const std::vector<BYTE> &anPixels );
            /// Writes linear RGB colors as a Radiance HDR file.
        static BOOL     WriteHdr( const std::string &strFile, int nWidth, int nHeight, const std::vector<float> &anPixels );

            /// The PNG file's bytes, and the zlib stream inside it (for tests).
        static std::vector<BYTE> EncodePng( int nWidth, int nHeight, const std::vector<BYTE> &anPixels );
        static std::vector<BYTE> Deflate( const std::vector<BYTE> &anData );
            /// A color as RGBE: a mantissa byte for each channel and a shared
            /// exponent. Negative, NaN and tiny channels become 0.
        static void     ToRgbe( float nRed, float nGreen, float nBlue, BYTE anRgbe[ 4 ] );
};

#endif // __IMAGEEXPORT_H__
