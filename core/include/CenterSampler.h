/*************************************************************************
*   CCenterSampler                                                       *
*                                                                        *
*   The default sampler: one ray through the center of each pixel. The   *
*   tag is the pixel itself.                                             *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __CENTERSAMPLER_H__
#define __CENTERSAMPLER_H__

#include "PixelSampler.h"

class CCenterSampler : public CPixelSampler
{
    protected:
        int                     m_nLeft;            // The tile being sampled.
        int                     m_nTop;
        int                     m_nWidth;
        int                     m_nHeight;
        int                     m_nX;
        int                     m_nY;

    public:
                                CCenterSampler( void );

        virtual CPixelSampler  *Clone( void ) const { return new CCenterSampler; }
        virtual const char     *GetName( void ) const { return "none"; }

        virtual long long       Begin( int nLeft, int nTop, int nWidth, int nHeight );
        virtual BOOL            NextRay( CScene &oScene, CRay &oRay );
        virtual void            PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor );
};

#endif // __CENTERSAMPLER_H__
