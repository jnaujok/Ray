/*************************************************************************
*   CImageWriter                                                         *
*                                                                        *
*   Writes finished pixels into the image file: each one is scaled by    *
*   the camera's exposure and run through its color model, and, for a    *
*   high dynamic range file, its linear color is kept too.               *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __IMAGEWRITER_H__
#define __IMAGEWRITER_H__

#include "PixelWriter.h"

class CColorModel;
class CImageFile;

class CImageWriter : public CPixelWriter
{
    protected:
        CImageFile         &m_oImageFile;
        const CColorModel  &m_oColorModel;
        FLT                 m_nExposure;

    public:
            /// nExposure scales every color before the color model (a real
            /// lens lets less light through than a pinhole).
                            CImageWriter( CImageFile &oImageFile, const CColorModel &oColorModel, FLT nExposure = 1.0 );

        virtual BOOL        PutPixel( int nX, int nY, const CColor &oColor );

    private:
                            CImageWriter( const CImageWriter & );
        CImageWriter       &operator =( const CImageWriter & );
};

#endif // __IMAGEWRITER_H__
