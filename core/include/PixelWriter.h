/*************************************************************************
*   CPixelWriter                                                         *
*                                                                        *
*   Where a pixel sampler puts the pixels it has finished, as linear     *
*   colors of any brightness. CImageWriter puts them in the image file   *
*   through the camera's color model; CTileTracer keeps a tile's pixels  *
*   to send back to the master.                                          *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __PIXELWRITER_H__
#define __PIXELWRITER_H__

#include "Globals.h"

class CColor;

class CPixelWriter
{
    public:
        virtual            ~CPixelWriter( void ) {}

            /// Writes pixel (nX, nY), in image coordinates, with this linear
            /// color. Returns FALSE if the pixel is outside what this writer
            /// holds or couldn't be written.
        virtual BOOL        PutPixel( int nX, int nY, const CColor &oColor ) = 0;
};

#endif // __PIXELWRITER_H__
