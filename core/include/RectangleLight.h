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

    public:
                        DYNAMIC_OBJECTS( CRectangleLight );

                        CRectangleLight( void );
        virtual        ~CRectangleLight( void );

        virtual CColor  GetShade( CVector oVector );
        virtual int     GetSampleCount( void ) { return m_nSamples; }
        virtual BOOL    SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint );

#ifdef RAY_GUI
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif
};

#endif // __RECTANGLELIGHT_H__
