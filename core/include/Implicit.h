/*************************************************************************
*   CImplicit                                                            *
*                                                                        *
*   A surface given by a formula: where f(x, y, z) = 0, inside where f   *
*   is negative, within a box of half-sizes "bounds" around the center   *
*   (in the shape's own frame, before "scale"). See Expression.h for     *
*   the language, and scenes/README.md.                                  *
*                                                                        *
*   A ray is followed through the box in steps, and the surface found    *
*   where f changes sign between two steps, then pinned down by          *
*   bisection to rounding. The step is the box's diagonal over "steps"   *
*   (default 256); with "lipschitz", the most f can change per unit of   *
*   distance, a step may be longer, |f| / lipschitz, where that is safe. *
*   A feature thinner than the short step can be stepped over, and a     *
*   place where f only touches zero without changing sign is missed.     *
*   The cost is up to "steps" evaluations of the formula a ray through   *
*   the box, plus about 60 where it hits.                                *
*                                                                        *
*   The normal is the gradient of f, worked out exactly. It is a solid   *
*   (inside the box), so a csg can use it.                               *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __IMPLICIT_H__
#define __IMPLICIT_H__

#include "FieldShape.h"
#include "Expression.h"

#include <string>
#include <utility>
#include <vector>

class CImplicit : public CFieldShape
{
    protected:
        std::string         m_strFormula;
        std::vector< std::pair<std::string, double> > m_aoParameters;  // in the scene's order
        CVector             m_oBounds;          // half-sizes
        double              m_anHalf[ 3 ];      // the same, as numbers
        BOOL                m_bBoundsNumber;    // given as one number
        int                 m_nSteps;
        FLT                 m_nLipschitz;       // 0: none
        CExpression         m_oExpression;

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
                            DYNAMIC_OBJECTS( CImplicit );

                            CImplicit( void );
        virtual            ~CImplicit( void );

            /// Sets the surface; FALSE (with RayJson::Fail) if the formula
            /// or the numbers are bad.
        BOOL                Set( const CVector &oCenter, const std::string &strFormula, const CVector &oBounds, int nSteps,
                                 FLT nLipschitz, FLT nScale );
        virtual BOOL        IsClosed( void ) { return TRUE; }
};

#endif // __IMPLICIT_H__
