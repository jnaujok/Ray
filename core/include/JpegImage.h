/*************************************************************************
*   CJpegImage                                                           *
*                                                                        *
*   Reads JPEG (JFIF and Exif) images, for image textures and the        *
*   images inside glTF models. Baseline and progressive Huffman-coded    *
*   files are read, with 8-bit samples, one component (grayscale) or     *
*   three (YCbCr, or RGB when the file says so), any chroma subsampling  *
*   the format allows, and restart markers. An Exif orientation is       *
*   applied, so the picture comes out the right way up.                  *
*                                                                        *
*   The pixels are worked out with integers only: the inverse DCT is the *
*   accurate integer one of the IJG's libjpeg ("islow"), chroma is       *
*   brought up to full size by libjpeg's "fancy" triangle filter for the *
*   usual 2:1 subsamplings (and copied for the others), and YCbCr is     *
*   turned into RGB with libjpeg's fixed-point formula. So the same file *
*   gives the same pixels on every platform and compiler, and networked  *
*   engines match threads byte for byte. Against libjpeg-turbo with its  *
*   defaults the result is the same or within a step or two.             *
*                                                                        *
*   Not read, each reported with a reason: arithmetic coding, lossless   *
*   and hierarchical JPEG, 12-bit samples, and four-component (CMYK or   *
*   YCCK) files. A damaged or cut-off file is reported, never read past  *
*   its end.                                                             *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __JPEGIMAGE_H__
#define __JPEGIMAGE_H__

#include "Globals.h"

#include <string>
#include <vector>

class CJpegImage
{
    public:
            /// Whether the data starts with a JPEG start-of-image marker.
        static BOOL     IsJpeg( const std::string &strData );

            /// Decodes a JPEG file's contents to 8-bit RGB pixels, top row
            /// first, turned upright by its Exif orientation. FALSE, with
            /// why in strError, if it isn't a JPEG Ray can read or it is
            /// damaged.
        static BOOL     Decode( const std::string &strData, int &nWidth, int &nHeight, std::vector<BYTE> &anPixels,
                                std::string &strError );
};

#endif // __JPEGIMAGE_H__
