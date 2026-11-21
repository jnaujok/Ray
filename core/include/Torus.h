/*************************************************************************
*   CTorus                                                               *
*                                                                        *
*   A ring: the circle of minorRadius swept around +y at majorRadius     *
*   from the center, turned by rotation. A solid, so a csg can use it.   *
*                                                                        *
*   A ray meets it where a quartic is zero. The quartic is set up from   *
*   where the ray enters the ring's bounding sphere, so a torus far from *
*   the scene's origin or far from the camera keeps its precision, and   *
*   each root is found by bisection (see CLocalShape::PolynomialRoots).  *
*                                                                        *
*   Surface coordinates: u is the angle around +y (as on a sphere), v    *
*   the angle around the tube, 0 on the outside rim, going up first.     *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __TORUS_H__
#define __TORUS_H__

#include "LocalShape.h"

class CTorus : public CLocalShape
{
    protected:
        FLT                 m_nMajor;
        FLT                 m_nMinor;

        virtual BOOL        PrepareShape( void );
        virtual BOOL        LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT );
        virtual void        LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] );
        virtual void        LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV );
        virtual void        LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] );
        virtual void        ShapeToJson( IASLib::CObjectNode &oNode );
        virtual BOOL        ShapeFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ShapeFields( void );
        virtual void        LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals );

    public:
                            DYNAMIC_OBJECTS( CTorus );

                            CTorus( void );
        virtual            ~CTorus( void );

            /// Sets the ring; FALSE if the radii are bad.
        BOOL                Set( const CVector &oCenter, FLT nMajor, FLT nMinor, const CVector &oRotationDegrees );
        virtual BOOL        IsClosed( void ) { return TRUE; }
};

#endif // __TORUS_H__
