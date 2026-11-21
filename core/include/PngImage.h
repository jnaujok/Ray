/*************************************************************************
*   CPngImage                                                            *
*                                                                        *
*   Reads PNG images, for image textures and normal maps. Every PNG the  *
*   standard allows is read: grayscale, RGB, palette, grayscale with     *
*   alpha and RGBA, at 1, 2, 4, 8 or 16 bits a channel, interlaced or    *
*   not. The pixels come out as 8-bit RGB: low bit depths are scaled up  *
*   to the full 0-255 range, 16-bit channels are rounded to 8 bits, and  *
*   alpha is dropped (the color is taken as it is, not blended with any  *
*   background).                                                         *
*                                                                        *
*   The compressed data is inflated by a small decoder of our own (the   *
*   deflate format of RFC 1951 inside the zlib wrapper of RFC 1950), so  *
*   no zlib is needed. Chunk CRCs and the zlib checksum are checked, and *
*   a damaged or truncated file is reported, never read past its end.    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __PNGIMAGE_H__
#define __PNGIMAGE_H__

#include "Globals.h"

#include <string>
#include <vector>

class CPngImage
{
    public:
            /// Whether the data starts with the PNG signature.
        static BOOL     IsPng( const std::string &strData );

            /// Decodes a PNG file's contents to 8-bit RGB pixels, top row
            /// first. FALSE, with why in strError, if it isn't a PNG Ray can
            /// read or it is damaged.
        static BOOL     Decode( const std::string &strData, int &nWidth, int &nHeight, std::vector<BYTE> &anPixels,
                                std::string &strError );

            /// Inflates a zlib stream (checking its header and checksum).
            /// FALSE, with why in strError, if it is damaged or would
            /// inflate to more than nLimit bytes.
        static BOOL     Inflate( const BYTE *pData, size_t nSize, std::vector<BYTE> &anOut, size_t nLimit,
                                 std::string &strError );
            /// Inflates raw deflate data (RFC 1951, no zlib header or
            /// checksum), as gzip holds it; *pnConsumed, if given, is how
            /// many bytes it took.
        static BOOL     InflateRaw( const BYTE *pData, size_t nSize, std::vector<BYTE> &anOut, size_t nLimit,
                                    std::string &strError, size_t *pnConsumed );
};

#endif // __PNGIMAGE_H__
