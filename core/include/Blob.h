/*************************************************************************
*   CBlob                                                                *
*                                                                        *
*   Metaballs: balls whose fields add up, so near ones melt together.    *
*   Each ball (a center, a radius and a strength, which may be negative  *
*   to dent the others) adds strength * ( 1 - r^2 / radius^2 )^3 within  *
*   its radius and nothing beyond it (Wyvill's falloff). The surface is  *
*   where the sum is "threshold" (default 0.5); inside, it is more. A    *
*   lone ball of strength s and radius R is a sphere of radius           *
*   R sqrt( 1 - ( threshold / s )^(1/3) ).                               *
*                                                                        *
*   Along a ray each ball's field is a polynomial of degree 6 in t       *
*   while the ray is inside its radius, so between the places where the  *
*   ray enters and leaves balls the sum is one polynomial, and its roots *
*   are found exactly (CLocalShape::PolynomialRoots). The normal is the  *
*   field's gradient. It is a solid, so a csg can use it.                *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __BLOB_H__
#define __BLOB_H__

#include "FieldShape.h"

#include <vector>

class CBlob : public CFieldShape
{
    public:
        struct SBall
        {
            double              anCenter[ 3 ];
            double              nRadius;
            double              nStrength;
        };

    protected:
        std::vector<SBall>  m_aoBalls;
        FLT                 m_nThreshold;
        double              m_anLow[ 3 ];       // the box the positive balls stay inside
        double              m_anHigh[ 3 ];

        virtual BOOL        PrepareField( void );
        virtual double      Field( const double anPoint[ 3 ] ) const;
        virtual BOOL        FieldHit( const double anOrigin[ 3 ], const double anDirection[ 3 ], double nLow, double nHigh,
                                      double &nT ) const;
        virtual void        FieldNormal( const double anPoint[ 3 ], double anNormal[ 3 ] ) const;
        virtual void        FieldBounds( double anLow[ 3 ], double anHigh[ 3 ] ) const;
        virtual void        FieldToJson( IASLib::CObjectNode &oNode );
        virtual BOOL        FieldFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ShapeFields( void );

    public:
                            DYNAMIC_OBJECTS( CBlob );

                            CBlob( void );
        virtual            ~CBlob( void );

            /// Sets the balls; FALSE (with RayJson::Fail) if they are bad.
        BOOL                Set( const CVector &oCenter, const std::vector<SBall> &aoBalls, FLT nThreshold );
            /// The sum of the balls' fields at a point (in the blob's own
            /// frame, at size 1).
        double              Sum( const double anPoint[ 3 ] ) const;
        virtual BOOL        IsClosed( void ) { return TRUE; }
};

#endif // __BLOB_H__
