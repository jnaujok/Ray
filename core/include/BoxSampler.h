/*************************************************************************
*   CBoxSampler                                                          *
*                                                                        *
*   Anti-aliasing with four rays per pixel, each halfway from the        *
*   pixel's center to one of its corners: at (1/4, 1/4), (3/4, 1/4),     *
*   (1/4, 3/4) and (3/4, 3/4) of the pixel, a small box around the       *
*   center. The four are averaged equally.                               *
*                                                                        *
*   The tags are positions on a grid of twice the image's resolution:    *
*   (2x + i, 2y + j) is ray (i, j) of pixel (x, y).                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __BOXSAMPLER_H__
#define __BOXSAMPLER_H__

#include "PixelSampler.h"
#include "PixelAccumulator.h"

class CBoxSampler : public CPixelSampler
{
    protected:
        int                     m_nWidth;
        int                     m_nHeight;
        int                     m_nPixel;           // The next pixel, in reading order.
        int                     m_nRay;             // The next of its four rays.
        CPixelAccumulator       m_oSums;

    public:
                                CBoxSampler( void );

        virtual CPixelSampler  *Clone( void ) const { return new CBoxSampler; }
        virtual const char     *GetName( void ) const { return "box"; }

        virtual long long       Begin( int nWidth, int nHeight );
        virtual BOOL            NextRay( CScene &oScene, CRay &oRay );
        virtual void            PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor );
        virtual void            End( void );
};

#endif // __BOXSAMPLER_H__
