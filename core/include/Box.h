/*************************************************************************
*   CBox                                                                 *
*                                                                        *
*   A rectangular box, or a cube: a center, a size along each of its own *
*   axes, and a rotation that turns those axes from the scene's x, y and *
*   z (about x, then y, then z, in degrees).                             *
*                                                                        *
*   Surface coordinates: each face holds the unit square once, upright   *
*   as seen from outside (the four sides with u to the right and v up;   *
*   the top as seen from above with +z up, the bottom from below with -z *
*   up, before the rotation).                                            *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __BOX_H__
#define __BOX_H__

#include "Objects.h"

class CBox : public CObjects
{
    protected:
        CVector             m_oCenter;
        CVector             m_oSize;
        CVector             m_oRotation;        // degrees about x, then y, then z
        CColor              m_oColor;

            // Worked out by Prepare: the box's own axes in the scene, and
            // half its size along each.
        CVector             m_aoAxes[ 3 ];
        FLT                 m_anHalf[ 3 ];

        BOOL                Prepare( void );
            // A point in the box's own frame, each axis scaled so the faces
            // are at -1 and 1.
        void                Local( CVector &oPoint, FLT anLocal[ 3 ] );
            // The face a point is on: the axis, and its sign.
        void                FaceAt( CVector &oPoint, int &nAxis, int &nSign, FLT anLocal[ 3 ] );

    public:
                            DYNAMIC_OBJECTS( CBox );

                            CBox( void );
        virtual            ~CBox( void );

            // Sets the shape; FALSE if a size isn't more than 0.
        BOOL                Set( const CVector &oCenter, const CVector &oSize, const CVector &oRotationDegrees );

        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect );
        virtual BOOL        SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV );
            // The outward normal at a point on the surface.
        CVector             NormalAt( CVector &oPoint );
        virtual CColor      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );

#ifdef RAY_GUI
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif
};

#endif // __BOX_H__
