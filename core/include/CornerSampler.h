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
*   The five are averaged equally by a CPixelAccumulator.                *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __CORNERSAMPLER_H__
#define __CORNERSAMPLER_H__

#include "PixelSampler.h"
#include "PixelAccumulator.h"

class CCornerSampler : public CPixelSampler
{
    protected:
        int                     m_nLeft;            // The tile being sampled.
        int                     m_nTop;
        int                     m_nWidth;
        int                     m_nHeight;
        int                     m_nGridX;           // Position of the next ray on the doubled grid.
        int                     m_nGridY;
        CPixelAccumulator       m_oSums;

    public:
                                CCornerSampler( void );

        virtual CPixelSampler  *Clone( void ) const { return new CCornerSampler; }
        virtual const char     *GetName( void ) const { return "corners"; }

        virtual long long       Begin( int nLeft, int nTop, int nWidth, int nHeight );
        virtual BOOL            NextRay( CScene &oScene, CRay &oRay );
        virtual void            PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor );
        virtual void            End( void );
};

#endif // __CORNERSAMPLER_H__
