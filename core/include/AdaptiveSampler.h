/*************************************************************************
*   CAdaptiveSampler                                                     *
*                                                                        *
*   Anti-aliasing that spends rays where they are needed: each pixel     *
*   gets at least "min" rays, then more, one at a time, until the        *
*   standard error of its brightness is below "threshold", or it has     *
*   "max". Flat areas (sky, a wall) stop early; edges, textures and      *
*   out-of-focus blur get the most rays. All rays in a pixel are         *
*   weighted equally.                                                    *
*                                                                        *
*   The rays follow the R2 sequence (Roberts, 2018), shifted for each    *
*   pixel by a hash of the pixel and the seed, so however many a pixel   *
*   gets they are spread evenly over it (and over a real lens, which     *
*   picks its point from where in the pixel the ray falls). The image is *
*   reproducible, and the same in every task.                            *
*                                                                        *
*   It decides on a pixel's next ray from the colors of the rays before  *
*   it, so it needs each ray's color (PutSample) before it makes the     *
*   next (NextRay), as CTileTracer does. The tags are the pixel's (x, y).*
*                                                                        *
*   JSON settings: "min" (default 9, at least 2), "max" (default 64, up  *
*   to 4096), "threshold" (default 0.01, a brightness on the scale where *
*   1 is white) and "seed" (default 0).                                  *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __ADAPTIVESAMPLER_H__
#define __ADAPTIVESAMPLER_H__

#include "PixelSampler.h"

class CAdaptiveSampler : public CPixelSampler
{
    protected:
        int                     m_nMinimum;
        int                     m_nMaximum;
        FLT                     m_nThreshold;
        int                     m_nSeed;
        int                     m_nLeft;            // The tile being sampled.
        int                     m_nTop;
        int                     m_nWidth;
        int                     m_nHeight;
        int                     m_nPixel;           // The pixel being sampled, in reading order.
        int                     m_nRays;            // Its rays so far.
        double                  m_adSum[ 3 ];       // their colors added up,
        double                  m_dBrightness;      // their brightnesses,
        double                  m_dSquares;         // and the brightnesses squared
        double                  m_adShift[ 2 ];     // the pixel's shift of the sequence

        void                    StartPixel( void );

    public:
                                CAdaptiveSampler( void );

        virtual CPixelSampler  *Clone( void ) const;
        virtual const char     *GetName( void ) const { return "adaptive"; }

        virtual long long       Begin( int nLeft, int nTop, int nWidth, int nHeight );
        virtual BOOL            NextRay( CScene &oScene, CRay &oRay );
        virtual void            PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor );
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __ADAPTIVESAMPLER_H__
