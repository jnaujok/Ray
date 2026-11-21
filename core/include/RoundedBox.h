/*************************************************************************
*   CRoundedBox                                                          *
*                                                                        *
*   A box with rounded edges and corners: a center, a size along each of *
*   its own axes (one number for a cube), a rotation as a box's, and the *
*   radius its edges are rounded to (at most half the smallest size; a   *
*   radius of half every size is a sphere). A solid, so a csg can use it. *
*                                                                        *
*   It is exactly a smaller box grown by the radius in every direction,  *
*   which is the union of three slabs, twelve edge cylinders and eight   *
*   corner spheres. Being convex, a ray is inside it from the earliest   *
*   of those parts it enters to the latest it leaves, so the hit is      *
*   found exactly with no stepping.                                      *
*                                                                        *
*   Surface coordinates: each face holds the unit square once, as on a   *
*   box (see Box.h), the rounded edges belonging to the nearest face.    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __ROUNDEDBOX_H__
#define __ROUNDEDBOX_H__

#include "LocalShape.h"

class CRoundedBox : public CLocalShape
{
    protected:
        CVector             m_oSize;
        FLT                 m_nRadius;
        FLT                 m_anHalf[ 3 ];      // half the size
        FLT                 m_anInner[ 3 ];     // half the size less the radius

        virtual BOOL        PrepareShape( void );
        virtual BOOL        LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT );
        virtual void        LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] );
        virtual void        LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV );
        virtual void        LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] );
        virtual BOOL        IsConvex( void ) { return TRUE; }
        virtual void        ShapeToJson( IASLib::CObjectNode &oNode );
        virtual BOOL        ShapeFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ShapeFields( void );
        virtual void        LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals );

    public:
                            DYNAMIC_OBJECTS( CRoundedBox );

                            CRoundedBox( void );
        virtual            ~CRoundedBox( void );

        BOOL                Set( const CVector &oCenter, const CVector &oSize, FLT nRadius, const CVector &oRotationDegrees );
        virtual BOOL        IsClosed( void ) { return TRUE; }
};

#endif // __ROUNDEDBOX_H__
