/*************************************************************************
*   CParaboloid                                                          *
*                                                                        *
*   A bowl: x^2 + z^2 = (radius^2 / height) y, from its vertex at the    *
*   center up +y to a rim of radius at height (before its rotation).     *
*   Capped, the rim is closed by a flat disc and it is a solid (convex,  *
*   so a csg can use it); open (the default), a dish seen from both      *
*   sides.                                                               *
*                                                                        *
*   Surface coordinates: on the bowl, u is the angle around +y (as on a  *
*   sphere) and v runs from 0 at the vertex to 1 at the rim; on the cap, *
*   the disc fills the unit square as seen from above.                   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __PARABOLOID_H__
#define __PARABOLOID_H__

#include "LocalShape.h"

class CParaboloid : public CLocalShape
{
    protected:
        FLT                 m_nRadius;
        FLT                 m_nHeight;
        BOOL                m_bCapped;
        FLT                 m_nK;               // radius^2 / height

        BOOL                OnCap( const FLT anPoint[ 3 ] ) const;

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
                            DYNAMIC_OBJECTS( CParaboloid );

                            CParaboloid( void );
        virtual            ~CParaboloid( void );

        virtual BOOL        IsClosed( void ) { return m_bCapped; }
};

#endif // __PARABOLOID_H__
