/*************************************************************************
*   CRandomSampler                                                       *
*                                                                        *
*   Anti-aliasing with one ray through the pixel's center, which counts  *
*   for half the pixel, and N more at random points in the pixel, which  *
*   share the other half.                                                *
*                                                                        *
*   The random points come from a hash of the pixel, the ray's number    *
*   and the seed, not from a generator that runs through the image, so   *
*   the same scene always gives the same image, whatever order the rays  *
*   are traced in, and threads and networked engines agree.              *
*                                                                        *
*   The tags are ( x * ( N + 1 ) + k, y ) for ray k of pixel (x, y);     *
*   ray 0 is the center.                                                 *
*                                                                        *
*   JSON settings: "samples", N (default 4, from 1 to 1024), and "seed"  *
*   (default 0).                                                         *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __RANDOMSAMPLER_H__
#define __RANDOMSAMPLER_H__

#include "PixelSampler.h"
#include "PixelAccumulator.h"

class CRandomSampler : public CPixelSampler
{
    protected:
        int                     m_nSamples;         // N: the random rays a pixel
        int                     m_nSeed;
        int                     m_nWidth;
        int                     m_nHeight;
        int                     m_nPixel;           // The next pixel, in reading order.
        int                     m_nRay;             // The next of its rays; 0 is the center.
        CPixelAccumulator       m_oSums;

    public:
        explicit                CRandomSampler( int nSamples = 4, int nSeed = 0 );

        virtual CPixelSampler  *Clone( void ) const { return new CRandomSampler( m_nSamples, m_nSeed ); }
        virtual const char     *GetName( void ) const { return "random"; }

        int                     GetSamples( void ) const { return m_nSamples; }
            /// N, from 1 to 1024; FALSE if it is out of range.
        BOOL                    SetSamples( int nSamples );
        int                     GetSeed( void ) const { return m_nSeed; }
        void                    SetSeed( int nSeed ) { m_nSeed = nSeed; }

            /// Where random ray nRay (from 1) of pixel (nX, nY) goes, from 0
            /// to 1 across and down the pixel.
        void                    Offset( int nX, int nY, int nRay, FLT &nU, FLT &nV ) const;

        virtual long long       Begin( int nWidth, int nHeight );
        virtual BOOL            NextRay( CScene &oScene, CRay &oRay );
        virtual void            PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor );
        virtual void            End( void );
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __RANDOMSAMPLER_H__
