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
*   Two opt-in refinements (added in 2026-alpha-0.20.1; leaving them out *
*   samples exactly as before):                                          *
*     "confidence" (default 0, off; else between 0.5 and 0.999): stop    *
*        once the half-width of that confidence interval for the mean,   *
*        Student's t for n - 1 degrees of freedom times the standard     *
*        error, is under "threshold". With few rays t is large (12.7 at  *
*        2 rays, 3.18 at 4 for 95%), so a handful of rays that happen to *
*        agree must agree much more closely before they are trusted,     *
*        though "min" should stay near 16: rays that agree exactly have  *
*        no spread for t to widen (measured in 2026-alpha-0.20.1).       *
*     "channels" ("brightness", the default, or "rgb"): measure the      *
*        error in red, green and blue apart and use the largest, so a    *
*        pixel whose color varies but whose brightness doesn't (a        *
*        spectral fringe) keeps sampling.                                *
*   Each pixel's ray count goes to CRayCountMap when it is recording     *
*   (raythrower --ray-count).                                            *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __ADAPTIVESAMPLER_H__
#define __ADAPTIVESAMPLER_H__

#include "PixelSampler.h"

#include <vector>

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
        FLT                     m_nConfidence;      // 0: plain standard error
        BOOL                    m_bChannels;        // measure red, green and blue apart
        double                  m_adChannel[ 3 ];   // each channel, capped, added up,
        double                  m_adChannelSquares[ 3 ];    // and squared
        std::vector<double>     m_adFactor;         // Student's t by ray count, when confidence is set

        void                    MakeFactors( void );

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

            /// Student's t at this two-sided confidence for this many degrees
            /// of freedom (a Cornish-Fisher expansion of the normal quantile;
            /// within 2% of the exact value from 3 degrees of freedom up).
        static double           StudentT( double dConfidence, int nFreedom );
};

#endif // __ADAPTIVESAMPLER_H__
