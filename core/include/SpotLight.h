/*************************************************************************
*   CSpotLight                                                           *
*                                                                        *
*   A light that shines in a cone: from its position along its           *
*   direction, out to "angle" degrees from the axis. The edge of the     *
*   pool of light fades over the last "falloff" degrees inside the cone  *
*   (smoothly, along a smoothstep), so a falloff of 0 gives a hard edge. *
*                                                                        *
*   With a "radius" it is a glowing disc of that radius rather than a    *
*   point: each surface it lights is shaded from "samples" points spread *
*   over the disc it shows (as a sphere light is), so its shadows are    *
*   soft. With no radius it is exactly a point light inside its cone.    *
*                                                                        *
*   Like the other lights, it gives the same light at every distance     *
*   (unless given an "attenuation"), and the camera doesn't see it. A    *
*   "profile" shapes the beam inside the cone, and a "filter" is         *
*   projected across it: see Light.h.                                    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __SPOTLIGHT_H__
#define __SPOTLIGHT_H__

#include "Light.h"

class CSpotLight : public CLight
{
    protected:
        CVector         m_oDirection;       // as given; normalized when used
        FLT             m_nAngle;           // degrees from the axis to the edge
        FLT             m_nFalloff;         // degrees, inside the edge, over which it fades
        FLT             m_nRadius;          // 0: a point
        int             m_nSamples;

    public:
                        DYNAMIC_OBJECTS( CSpotLight );

                        CSpotLight( void );
        virtual        ~CSpotLight( void );

            /// The light reaching a surface along oToLight (from the surface
            /// toward the light): the color, faded by where the surface is in
            /// the cone, and black outside it.
        virtual CColor  GetShade( CVector oToLight );
            /// How much of the light reaches a point seen along oToLight, from
            /// 0 (outside the cone) to 1 (inside the fading edge).
        FLT             ConeFactor( const CVector &oToLight ) const;
        virtual int     GetSampleCount( void ) { return ( m_nRadius > 0.0 ) ? m_nSamples : 0; }
        virtual BOOL    SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint );

    protected:
            // The filter across the cone: (u, v) from 0 to 1 over the square
            // around the cone's circle; a solid texture's point is the same
            // place, the cone's edge "size" from its middle.
        virtual void    FilterAt( const CVector &oPoint, const CVector &oToLight, STexturePoint &oAt ) const;

#ifdef RAY_GUI
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif
};

#endif // __SPOTLIGHT_H__
