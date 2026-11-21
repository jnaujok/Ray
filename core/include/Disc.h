/*************************************************************************
*   CDisc                                                                *
*                                                                        *
*   A flat disc of radius about the center, facing +y before its         *
*   rotation, with an optional hole of innerRadius (an annulus; see      *
*   CAnnulus). Shaded on whichever side a ray sees.                      *
*                                                                        *
*   Surface coordinates: u is the angle around +y (as on a sphere), v    *
*   runs from 0 at the inner edge (or the center) to 1 at the rim.       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __DISC_H__
#define __DISC_H__

#include "LocalShape.h"

class CDisc : public CLocalShape
{
    protected:
        FLT                 m_nRadius;
        FLT                 m_nInner;

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
                            DYNAMIC_OBJECTS( CDisc );

                            CDisc( void );
        virtual            ~CDisc( void );

        BOOL                Set( const CVector &oCenter, FLT nRadius, FLT nInner, const CVector &oRotationDegrees );
};

#endif // __DISC_H__
