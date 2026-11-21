/*************************************************************************
*   CSuperquadric                                                        *
*                                                                        *
*   A superellipsoid: ( |x/a|^(2/e2) + |z/c|^(2/e2) )^(e2/e1) +          *
*   |y/b|^(2/e1) <= 1, with radius a, b, c along its own x, y and z      *
*   (one number for all three) and two exponents: e1 shapes it from pole *
*   to pole (along y), e2 around y. Both 1 is an ellipsoid (a sphere for *
*   equal radii); near 0, a box with sharp edges; 2, a double cone or an *
*   octahedron; above 2, pinched star shapes. A solid, so a csg can use  *
*   it.                                                                  *
*                                                                        *
*   A ray is clipped to the bounding box, and the surface found where    *
*   G = F^(e1/2) - 1 changes sign (G grows like the distance from the    *
*   center, so it is well behaved). With both exponents at most 2 the    *
*   shape is convex, so G along a ray falls then rises: its lowest point *
*   is found by golden-section search, and the surface by bisection on   *
*   either side, so grazing rays are found too. Above 2 the ray is       *
*   stepped 256 times through the box instead, which can miss parts      *
*   thinner than a step.                                                 *
*                                                                        *
*   Surface coordinates: u is the angle around y (as on a sphere), v     *
*   the latitude, 0 at the bottom to 1 at the top.                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __SUPERQUADRIC_H__
#define __SUPERQUADRIC_H__

#include "LocalShape.h"

class CSuperquadric : public CLocalShape
{
    protected:
        CVector             m_oRadius;
        FLT                 m_nE1;
        FLT                 m_nE2;
        FLT                 m_anRadius[ 3 ];

            // G: below 0 inside, above outside.
        double              Field( const double anPoint[ 3 ] ) const;
        double              FieldAlong( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], double t ) const;
            // Where the field crosses 0 between a (field fa) and b.
        double              Bisect( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], double a, double b, double fa ) const;

        virtual BOOL        PrepareShape( void );
        virtual BOOL        LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT );
        virtual void        LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] );
        virtual void        LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV );
        virtual void        LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] );
        virtual BOOL        IsConvex( void ) { return ( m_nE1 <= 2.0 && m_nE2 <= 2.0 ) ? TRUE : FALSE; }
        virtual void        ShapeToJson( IASLib::CObjectNode &oNode );
        virtual BOOL        ShapeFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ShapeFields( void );
        virtual void        LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals );

    public:
                            DYNAMIC_OBJECTS( CSuperquadric );

                            CSuperquadric( void );
        virtual            ~CSuperquadric( void );

        BOOL                Set( const CVector &oCenter, const CVector &oRadius, FLT nE1, FLT nE2, const CVector &oRotationDegrees );
        virtual BOOL        IsClosed( void ) { return TRUE; }
};

#endif // __SUPERQUADRIC_H__
