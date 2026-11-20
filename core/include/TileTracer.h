/*************************************************************************
*   CTileTracer                                                          *
*                                                                        *
*   Traces one tile of an image, the work unit a task asks the master    *
*   for: runs the camera's pixel sampler over the tile, makes each       *
*   primary ray (through the lens, once for each channel with a          *
*   chromatic one), traces it, and keeps the finished pixels.            *
*                                                                        *
*   The pixels are linear colors before the camera's exposure, as        *
*   doubles. Threads and networked engines both trace with this, and     *
*   doubles cross the wire exactly, so they make the same image.         *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __TILETRACER_H__
#define __TILETRACER_H__

#include "PixelWriter.h"

#include <vector>

class CScene;

    // A rectangle of the image, in pixels.
struct STile
{
    int nLeft;
    int nTop;
    int nWidth;
    int nHeight;
};

class CTileTracer : public CPixelWriter
{
    protected:
        STile                   m_oTile;
        std::vector<double>    *m_pColors;

    public:
                                CTileTracer( void );

            /// Traces oTile of the scene. anColors gets its pixels row by
            /// row from the top left, red, green and blue. Returns the number
            /// of primary rays traced.
        long long               Trace( CScene &oScene, const STile &oTile, std::vector<double> &anColors );

            /// Where the sampler puts the tile's pixels.
        virtual BOOL            PutPixel( int nX, int nY, const CColor &oColor );

    private:
                                CTileTracer( const CTileTracer & );
        CTileTracer            &operator =( const CTileTracer & );
};

#endif // __TILETRACER_H__
