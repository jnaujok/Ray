/*************************************************************************
*   CCornerSampler                                                       *
*                                                                        *
*   Anti-aliasing with five rays per pixel: one at each corner and one   *
*   at the center, averaged. A corner is traced once and shared by the   *
*   four pixels around it, so an image costs about two rays per pixel.   *
*                                                                        *
*   The tags are positions on a grid of twice the image's resolution:    *
*   (2x + 1, 2y + 1) is the center of pixel (x, y), and (2i, 2j) is the  *
*   corner at the top left of pixel (i, j). Rays are made a grid row at  *
*   a time, a row of corners and then a row of centers, so the samples   *
*   of a pixel go out close together.                                    *
*                                                                        *
*   Samples are averaged unclipped, so the color model sees the true     *
*   brightness of each pixel, and summed in fixed point, so the image    *
*   doesn't depend on the order they come back in.                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __CORNERSAMPLER_H__
#define __CORNERSAMPLER_H__

#include "PixelSampler.h"

#include <cstdint>
#include <vector>

class CCornerSampler : public CPixelSampler
{
    protected:
            // The samples a pixel has received so far.
        struct SPixelSum
        {
            uint64_t            nRed;
            uint64_t            nGreen;
            uint64_t            nBlue;
            uint32_t            nSamples;
        };

        int                     m_nWidth;
        int                     m_nHeight;
        int                     m_nGridX;           // Position of the next ray on the doubled grid.
        int                     m_nGridY;
        std::vector<SPixelSum>  m_aoSums;

        void                    AddToPixel( CPixelWriter &oWriter, int nX, int nY, const uint64_t anColor[ 3 ] );

    public:
                                CCornerSampler( void );

        virtual CPixelSampler  *Clone( void ) const { return new CCornerSampler; }
        virtual const char     *GetName( void ) const { return "corners"; }

        virtual long long       Begin( int nWidth, int nHeight );
        virtual BOOL            NextRay( CScene &oScene, CRay &oRay );
        virtual void            PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor );
        virtual void            End( void );
};

#endif // __CORNERSAMPLER_H__
