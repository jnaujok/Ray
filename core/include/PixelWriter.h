/*************************************************************************
*   CPixelWriter                                                         *
*                                                                        *
*   Where a pixel sampler puts the pixels it has finished: runs each     *
*   one through the camera's color model and writes the bytes into the   *
*   image file.                                                          *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __PIXELWRITER_H__
#define __PIXELWRITER_H__

#include "Globals.h"

class CColor;
class CColorModel;
class CImageFile;

class CPixelWriter
{
    protected:
        CImageFile         &m_oImageFile;
        const CColorModel  &m_oColorModel;
        FLT                 m_nExposure;

    public:
            /// nExposure scales every color before the color model (a real
            /// lens lets less light through than a pinhole).
                            CPixelWriter( CImageFile &oImageFile, const CColorModel &oColorModel, FLT nExposure = 1.0 );

            /// Writes pixel (nX, nY) with this linear color. Returns FALSE if
            /// the pixel is outside the image or couldn't be written.
        BOOL                PutPixel( int nX, int nY, const CColor &oColor );

    private:
                            CPixelWriter( const CPixelWriter & );
        CPixelWriter       &operator =( const CPixelWriter & );
};

#endif // __PIXELWRITER_H__
