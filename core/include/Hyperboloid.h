/*************************************************************************
*   CHyperboloid                                                         *
*                                                                        *
*   A hyperboloid of one sheet, like a cooling tower: x^2 + z^2 =        *
*   radius^2 + k y^2, centered on the center and running height along +y *
*   (half above, half below, before its rotation), with radius at its    *
*   waist and topRadius at both ends (equal radii make a cylinder).      *
*   Capped, the ends are closed by flat discs and it is a solid; open    *
*   (the default), a shell seen from both sides.                         *
*                                                                        *
*   Surface coordinates: on the side, u is the angle around +y (as on a  *
*   sphere) and v runs from 0 at the bottom to 1 at the top; on a cap,   *
*   the disc fills the unit square as seen from outside.                 *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __HYPERBOLOID_H__
#define __HYPERBOLOID_H__

#include "LocalShape.h"

class CHyperboloid : public CLocalShape
{
    protected:
        FLT                 m_nRadius;
        FLT                 m_nTopRadius;
        FLT                 m_nHeight;
        BOOL                m_bCapped;
        FLT                 m_nK;               // ( topRadius^2 - radius^2 ) / ( height / 2 )^2

            // -1 on the bottom cap, 1 on the top, 0 on the side.
        int                 CapAt( const FLT anPoint[ 3 ] ) const;

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
                            DYNAMIC_OBJECTS( CHyperboloid );

                            CHyperboloid( void );
        virtual            ~CHyperboloid( void );

        virtual BOOL        IsClosed( void ) { return m_bCapped; }
};

#endif // __HYPERBOLOID_H__
