/*************************************************************************
*   CWebpImage                                                           *
*                                                                        *
*   Reads WebP images, for image textures and the images inside glTF     *
*   models (glTF's EXT_texture_webp). The RIFF container is read in its  *
*   simple and extended (VP8X) forms, with either kind of image data:    *
*                                                                        *
*   - lossless (VP8L, RFC 9649): every transform (predictor, color,      *
*     subtract-green and color-indexing), the color cache and meta       *
*     prefix codes; the pixels come out exactly as they went in;         *
*   - lossy (VP8 key frames, RFC 6386): the boolean decoder, all intra   *
*     prediction modes, segments, the DCT and Walsh-Hadamard transforms  *
*     and the normal and simple loop filters, then YUV 4:2:0 to RGB the  *
*     way libwebp does it by default ("fancy" upsampling and its         *
*     fixed-point conversion), so the pixels match libwebp's;            *
*                                                                        *
*   and the alpha (ALPH) chunk of a lossy image, raw or losslessly       *
*   compressed, with its filtering undone. Everything is integer         *
*   arithmetic, so every platform decodes the same pixels.               *
*                                                                        *
*   Ray's textures are RGB: alpha comes back separately if asked for,    *
*   and is otherwise dropped, as for PNG. Animated WebPs aren't read     *
*   (reported, with a reason). A damaged or cut-off file is reported,    *
*   never read past its end.                                             *
*                                                                        *
*   The VP8 probability and quantizer tables are those of RFC 6386 (the  *
*   reference decoder's source, under its BSD-style license; see         *
*   NOTICE).                                                             *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __WEBPIMAGE_H__
#define __WEBPIMAGE_H__

#include "Globals.h"

#include <string>
#include <vector>

class CWebpImage
{
    public:
            /// Whether the data is a RIFF file of type WEBP.
        static BOOL     IsWebp( const std::string &strData );

            /// Decodes a WebP file's contents to 8-bit RGB pixels, top row
            /// first, and its alpha (255 where it has none) into *pAlpha
            /// if given. FALSE, with why in strError, if it isn't a WebP
            /// Ray can read or it is damaged.
        static BOOL     Decode( const std::string &strData, int &nWidth, int &nHeight, std::vector<BYTE> &anPixels,
                                std::string &strError, std::vector<BYTE> *pAlpha = NULL );
};

#endif // __WEBPIMAGE_H__
