/*************************************************************************
*   CRectangleLight                                                      *
*                                                                        *
*   A glowing panel: an area light that casts soft shadows. It is the    *
*   parallelogram centered on its position with sides edge1 and edge2,   *
*   and it shines only to the side edge1 x edge2 points to (a ceiling    *
*   panel lights the room, not the ceiling). Each surface it lights is   *
*   shaded from points spread over it, each with its own shadow ray.     *
*                                                                        *
*   Like a point light, it gives the same light at every distance and    *
*   in every direction it shines, and the camera doesn't see it. Its     *
*   color is the whole panel's, shared among its samples.                *
*                                                                        *
*   With an "angle" (degrees from its facing; default 0, no limit) it    *
*   shines in a beam, like a softbox with a grid: its edge fades over    *
*   the last "falloff" degrees, shaped by a "profile" (see Light.h). It  *
*   also takes an "attenuation" and a "filter", as every light does.     *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __RECTANGLELIGHT_H__
#define __RECTANGLELIGHT_H__

#include "Light.h"

class CRectangleLight : public CLight
{
    protected:
        CVector         m_oEdge1;
        CVector         m_oEdge2;
        int             m_nSamples;
        FLT             m_nAngle;           // degrees from its facing to the edge of its beam; 0: all its side
        FLT             m_nFalloff;         // degrees, inside the edge, over which the beam fades

    public:
                        DYNAMIC_OBJECTS( CRectangleLight );

                        CRectangleLight( void );
        virtual        ~CRectangleLight( void );

        virtual CColor  GetShade( CVector oVector );
        virtual int     GetSampleCount( void ) { return m_nSamples; }
        virtual BOOL    SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint );

    protected:
            // The filter along its facing: a point projected onto its plane;
            // (u, v) are its place across the rectangle (0 to 1 over it); a
            // solid texture reads that place in units of "size".
        virtual void    FilterAt( const CVector &oPoint, const CVector &oToLight, STexturePoint &oAt ) const;

    public:
#ifdef RAY_GUI
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif
};

#endif // __RECTANGLELIGHT_H__
