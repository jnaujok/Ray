/*************************************************************************
*   CStratifiedSampler                                                   *
*                                                                        *
*   Anti-aliasing with an n by n grid of rays per pixel: the pixel is    *
*   cut into n x n equal cells and one ray goes through a random point   *
*   in each, all weighted equally. Spreading the rays this way gives     *
*   smoother results than purely random points, and equal weights suit   *
*   effects that need many rays a pixel, such as a real lens's depth of  *
*   field.                                                               *
*                                                                        *
*   As in CRandomSampler, the points come from a hash of the pixel, the  *
*   ray's number and the seed, so the image is reproducible.             *
*                                                                        *
*   The tags are ( x * n * n + k, y ) for ray k of pixel (x, y).         *
*                                                                        *
*   JSON settings: "grid", n (default 4, so 16 rays; from 1 to 32), and  *
*   "seed" (default 0).                                                  *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __STRATIFIEDSAMPLER_H__
#define __STRATIFIEDSAMPLER_H__

#include "PixelSampler.h"
#include "PixelAccumulator.h"

class CStratifiedSampler : public CPixelSampler
{
    protected:
        int                     m_nGrid;            // n: rays a pixel along each side
        int                     m_nSeed;
        int                     m_nWidth;
        int                     m_nHeight;
        int                     m_nPixel;           // The next pixel, in reading order.
        int                     m_nRay;             // The next of its rays.
        CPixelAccumulator       m_oSums;

    public:
        explicit                CStratifiedSampler( int nGrid = 4, int nSeed = 0 );

        virtual CPixelSampler  *Clone( void ) const { return new CStratifiedSampler( m_nGrid, m_nSeed ); }
        virtual const char     *GetName( void ) const { return "stratified"; }

        int                     GetGrid( void ) const { return m_nGrid; }

        virtual long long       Begin( int nWidth, int nHeight );
        virtual BOOL            NextRay( CScene &oScene, CRay &oRay );
        virtual void            PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor );
        virtual void            End( void );
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __STRATIFIEDSAMPLER_H__
