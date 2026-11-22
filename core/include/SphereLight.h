/*************************************************************************
*   CSphereLight                                                         *
*                                                                        *
*   A glowing ball: an area light that casts soft shadows. Each surface  *
*   it lights is shaded from points spread over the disc the ball shows  *
*   it (its outline as seen from there), each with its own shadow ray,   *
*   so a shadow is dark where none of the ball can be seen, and fades    *
*   through a penumbra where some of it can.                             *
*                                                                        *
*   Like a point light, it gives the same light at every distance and    *
*   in every direction, and the camera doesn't see it. Its color is the  *
*   whole ball's, shared among its samples.                              *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __SPHERELIGHT_H__
#define __SPHERELIGHT_H__

#include "Light.h"

class CSphereLight : public CLight
{
    protected:
        FLT             m_nRadius;
        int             m_nSamples;

    public:
        virtual int     ShadowRayKind( void ) const { return CRayCounts::SHADOW_AREA; }
                        DYNAMIC_OBJECTS( CSphereLight );

                        CSphereLight( void );
        virtual        ~CSphereLight( void );

        virtual CColor  GetShade( CVector oVector );
        virtual int     GetSampleCount( void ) { return m_nSamples; }
        virtual BOOL    SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint );
        FLT             GetRadius( void ) const { return m_nRadius; }

#ifdef RAY_GUI
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif
};

#endif // __SPHERELIGHT_H__
