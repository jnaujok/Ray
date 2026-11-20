/*************************************************************************
*   CCone                                                                *
*                                                                        *
*   A cone, or a truncated one, about the line from its base to its top: *
*   a circle of baseRadius around the base and one of topRadius around   *
*   the top (0 for a point), joined by a straight side. Capped, the ends *
*   are closed with flat discs and the cone is a solid; uncapped it is   *
*   an open shell, which can be seen into and shadows itself inside.     *
*   CCylinder is the same with equal radii.                              *
*                                                                        *
*   Surface coordinates: on the side, u is the angle about the axis (as  *
*   on a sphere, for a cone standing on +y) and v runs from 0 at the     *
*   base to 1 at the top; on a cap, the disc fills the unit square as    *
*   seen from outside the cap.                                           *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __CONE_H__
#define __CONE_H__

#include "Objects.h"

class CCone : public CObjects
{
    protected:
        CVector             m_oBase;
        CVector             m_oTop;
        FLT                 m_nBaseRadius;
        FLT                 m_nTopRadius;
        BOOL                m_bCapped;
        CColor              m_oColor;

            // Worked out from the above by Prepare.
        CVector             m_oAxis;            // unit vector, base to top
        CVector             m_oAcross1;         // unit vectors across the axis,
        CVector             m_oAcross2;         // m_oAcross1 x m_oAxis
        FLT                 m_nHeight;
        FLT                 m_nSlope;           // how fast the radius shrinks along the axis
        FLT                 m_nMinimumT;        // closer hits are the surface a ray leaves from

            // Works out the axis and the frame across it. FALSE (with
            // RayJson::Fail) if the base and top are the same point or the
            // radii are bad.
        BOOL                Prepare( void );
            // The nearest hit with nMinimumT < t < the ray's t, or FALSE.
        BOOL                Hit( CRay &oRay, FLT nMinimumT, FLT &nT );
            // Where a point is: its distance along the axis from the base,
            // and across it in the frame.
        void                Local( CVector &oPoint, FLT &nAlong, FLT &nAcross1, FLT &nAcross2 );
            // Which part a point on the surface is on: 0 the side, 1 the
            // base cap, 2 the top cap.
        int                 PartAt( CVector &oPoint );
            // The fields both cones and cylinders have.
        BOOL                ReadEnds( const IASLib::CJsonNode &oNode );

    public:
                            DYNAMIC_OBJECTS( CCone );

                            CCone( void );
        virtual            ~CCone( void );

            // Sets the shape; FALSE if it isn't one (see Prepare).
        BOOL                Set( const CVector &oBase, const CVector &oTop, FLT nBaseRadius, FLT nTopRadius, BOOL bCapped );

        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect );
        virtual BOOL        GetBounds( CVector &oMinimum, CVector &oMaximum );
        virtual BOOL        SurfaceNormal( CVector &oPoint, CVector &oNormal ) { oNormal = NormalAt( oPoint ); return TRUE; }
        virtual BOOL        IsClosed( void ) { return m_bCapped; }
            // A capped cone is convex, so a ray leaving it can't hit it
            // again where it is lit; an open one can (its inside), so it is
            // tested past the point the ray leaves from.
        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore );
        virtual BOOL        SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV );
            // The outward normal at a point on the surface.
        CVector             NormalAt( CVector &oPoint );
        virtual CColor      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );

#ifdef RAY_GUI
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif
};

#endif // __CONE_H__
