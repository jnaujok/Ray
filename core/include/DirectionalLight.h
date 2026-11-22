/*************************************************************************
*   CDirectionalLight                                                    *
*                                                                        *
*   Sunlight: light from so far away that it arrives everywhere along    *
*   the same direction ("direction", the way the light travels), and     *
*   its shadows are the same size as what casts them.                    *
*                                                                        *
*   With an "angle" (the sun's angular diameter in degrees: the real sun *
*   is about 0.53) it is a disc in the sky rather than a point: each     *
*   surface is shaded from "samples" directions spread over that disc,   *
*   each with its own shadow ray, so shadows sharpen near what casts     *
*   them and blur farther away, as the sun's do.                         *
*                                                                        *
*   A shadow ray toward it goes RAY_FAR_AWAY scene units, which is far   *
*   past anything in a scene.                                            *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __DIRECTIONALLIGHT_H__
#define __DIRECTIONALLIGHT_H__

#include "Light.h"

class CDirectionalLight : public CLight
{
    protected:
        CVector         m_oDirection;       // the way the light travels, as given
        FLT             m_nAngle;           // degrees across the disc; 0: a point
        int             m_nSamples;

    public:
        virtual int     ShadowRayKind( void ) const { return CRayCounts::SHADOW_SUN; }
                        DYNAMIC_OBJECTS( CDirectionalLight );

                        CDirectionalLight( void );
        virtual        ~CDirectionalLight( void );

        virtual CColor  GetShade( CVector oToLight );
        virtual CVector ToLight( CVector &oFrom );
        virtual int     GetSampleCount( void ) { return ( m_nAngle > 0.0 ) ? m_nSamples : 0; }
        virtual BOOL    SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint );

    protected:
            // The filter straight along the light: a point projected onto
            // the plane across it; (u, v) are its place in units of "size",
            // 0.5 on the line through the origin.
        virtual void    FilterAt( const CVector &oPoint, const CVector &oToLight, STexturePoint &oAt ) const;

    public:
#ifdef RAY_GUI
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif
};

#endif // __DIRECTIONALLIGHT_H__
