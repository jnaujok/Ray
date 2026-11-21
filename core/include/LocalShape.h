/*************************************************************************
*   CLocalShape                                                          *
*                                                                        *
*   The base of the shapes worked out in a frame of their own: a center  *
*   and a rotation that turns the shape's x, y and z from the scene's    *
*   (about x, then y, then z, in degrees, as a box's rotation does). In  *
*   its own frame each shape stands on +y: a torus lies around it, a     *
*   disc faces it, a paraboloid opens up it.                             *
*                                                                        *
*   A subclass answers questions in its own frame (where a ray hits,     *
*   the normal, surface coordinates, a bounding box, triangles for the   *
*   editor); this class turns rays and points into that frame and back,  *
*   and reads and writes the fields every shape has (center, rotation,   *
*   color, materials, bump). The frame is orthonormal, so a ray's t is   *
*   the same in both.                                                    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __LOCALSHAPE_H__
#define __LOCALSHAPE_H__

#include "Objects.h"

#include <vector>

class CLocalShape : public CObjects
{
    protected:
        CVector             m_oCenter;
        CVector             m_oRotation;        // degrees about x, then y, then z
        CColor              m_oColor;

            // Worked out by Prepare: the shape's own axes in the scene.
        CVector             m_aoAxes[ 3 ];
        FLT                 m_anAxes[ 3 ][ 3 ]; // the same, as numbers
        FLT                 m_nMinimumT;        // closer hits are the surface a ray leaves from

            // Turns the frame and asks the subclass to check and prepare
            // its own fields. FALSE (with RayJson::Fail) if they are bad.
        BOOL                Prepare( void );
        void                ToLocal( CVector &oPoint, FLT anLocal[ 3 ] );
        void                DirectionToLocal( CVector &oDirection, FLT anLocal[ 3 ] );
        CVector             FromLocalDirection( const FLT anLocal[ 3 ] );
        CVector             FromLocalPoint( const FLT anLocal[ 3 ] );
            // The nearest hit with nMinimumT < t < the ray's t, or FALSE.
        BOOL                Hit( CRay &oRay, FLT nMinimumT, FLT &nT );

            // What each shape answers, in its own frame. anDirection is a
            // unit vector; LocalHit gives the nearest t in (nLow, nHigh).
        virtual BOOL        PrepareShape( void ) = 0;
        virtual BOOL        LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT ) = 0;
        virtual void        LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] ) = 0;
        virtual void        LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV ) = 0;
        virtual void        LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] ) = 0;
            // Whether the shape is a convex solid, so a ray leaving its
            // surface can't meet it again.
        virtual BOOL        IsConvex( void ) { return FALSE; }
            // The fields a shape adds, and its own field names (NULL ended).
        virtual void        ShapeToJson( IASLib::CObjectNode &oNode ) = 0;
        virtual BOOL        ShapeFromJson( const IASLib::CJsonNode &oNode ) = 0;
        virtual const char *const *ShapeFields( void ) = 0;
            // Triangles approximating the shape in its own frame (three
            // corners each, with normals), for the editor's preview.
        virtual void        LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals ) = 0;
            // Whether the scene must give "center" (a shape made of patches
            // whose points are where they are needs none).
        virtual BOOL        CenterRequired( void ) { return TRUE; }

    public:
                            CLocalShape( void );
        virtual            ~CLocalShape( void );

        virtual void        ToJson( IASLib::CObjectNode &oNode );
        virtual BOOL        FromJson( const IASLib::CJsonNode &oNode );

        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect );
        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore );
        virtual BOOL        GetBounds( CVector &oMinimum, CVector &oMaximum );
        virtual BOOL        SurfaceNormal( CVector &oPoint, CVector &oNormal ) { oNormal = NormalAt( oPoint ); return TRUE; }
        virtual BOOL        SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV );
            /// The outward normal at a point on the surface.
        CVector             NormalAt( CVector &oPoint );
        virtual CColor      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );
            /// Triangles approximating the shape where it is in the scene,
            /// for the editor: nine numbers each (three corners), and a
            /// normal at each corner.
        void                Triangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals );

            /// Turns anVector about x, then y, then z, by the angles in
            /// radians, as a box's rotation does.
        static void         RotateXYZ( FLT anVector[ 3 ], FLT nX, FLT nY, FLT nZ );
            /// The real roots of the polynomial anCoeffs[ 0 ] + anCoeffs[ 1 ] x
            /// + ... + anCoeffs[ nDegree ] x^nDegree between nLow and nHigh,
            /// in increasing order: each found by bisection between the
            /// turning points (the roots of the derivative, found the same
            /// way), so it is exact to rounding. A root where the polynomial
            /// only touches zero without crossing is missed.
        static int          PolynomialRoots( const double *anCoeffs, int nDegree, double nLow, double nHigh, double *anRoots );

#ifdef RAY_GUI
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif
};

    // Helpers the shapes share for the editor's triangles.
namespace LocalMesh
{
        // Adds the triangle a, b, c with normals na, nb, nc.
    void    Add( std::vector<FLT> &anPositions, std::vector<FLT> &anNormals, const FLT a[ 3 ], const FLT b[ 3 ], const FLT c[ 3 ],
                 const FLT na[ 3 ], const FLT nb[ 3 ], const FLT nc[ 3 ] );
        // Adds a surface of revolution about +y: the profile (radius,
        // height, normal's radial and up parts) at nPoints places, swept
        // around nSegments times.
    void    Revolve( std::vector<FLT> &anPositions, std::vector<FLT> &anNormals, const std::vector<FLT> &anProfile, int nSegments );
        // Adds a flat ring at height nY between two radii, facing +y if
        // bUp, else -y.
    void    Ring( std::vector<FLT> &anPositions, std::vector<FLT> &anNormals, FLT nY, FLT nInner, FLT nOuter, BOOL bUp, int nSegments );
}

#endif // __LOCALSHAPE_H__
