/*************************************************************************
*   CFractal                                                             *
*                                                                        *
*   A fractal, found by sphere tracing a distance estimate: a number at  *
*   each point no larger than the distance to the surface, so a ray can  *
*   step that far safely, until it is closer than "epsilon".             *
*                                                                        *
*   "kind" picks the fractal:                                            *
*                                                                        *
*       mandelbulb  White and Nylander's 3D Mandelbrot set: z -> z^power *
*                   + c in spherical coordinates (+y the pole). Fields   *
*                   power (8), iterations (10), bailout (2). Within a    *
*                   box of half-size 1.25.                               *
*       menger      The Menger sponge: a cube of half-size 1, its middle *
*                   thirds drilled out "iterations" (3) times. With 0 it *
*                   is the cube.                                         *
*       sierpinski  The Sierpinski tetrahedron, corners at (1, 1, 1),   *
*                   (-1, -1, 1), (-1, 1, -1) and (1, -1, -1), folded     *
*                   "iterations" (6) times. With 0 it is the solid      *
*                   tetrahedron.                                         *
*                                                                        *
*   Sphere tracing takes up to "maxSteps" (256) steps a ray, each of     *
*   "stepFactor" (1) times the estimate; the surface is where the        *
*   estimate drops below "epsilon" (0.0001, at size 1). A smaller        *
*   epsilon shows finer detail and costs more steps. The normal is the   *
*   estimate's gradient, by differences. Sphere tracing can't start      *
*   inside, so a fractal is not a solid a csg can use.                   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __FRACTAL_H__
#define __FRACTAL_H__

#include "FieldShape.h"

class CFractal : public CFieldShape
{
    public:
        enum EKind { KIND_MANDELBULB = 0, KIND_MENGER = 1, KIND_SIERPINSKI = 2 };

    protected:
        int                 m_nKind;
        int                 m_nIterations;
        FLT                 m_nPower;
        FLT                 m_nBailout;
        FLT                 m_nEpsilon;
        int                 m_nMaxSteps;
        FLT                 m_nStepFactor;

        virtual BOOL        PrepareField( void );
        virtual double      Field( const double anPoint[ 3 ] ) const;
        virtual BOOL        FieldHit( const double anOrigin[ 3 ], const double anDirection[ 3 ], double nLow, double nHigh,
                                      double &nT ) const;
        virtual void        FieldNormal( const double anPoint[ 3 ], double anNormal[ 3 ] ) const;
        virtual void        FieldBounds( double anLow[ 3 ], double anHigh[ 3 ] ) const;
        virtual void        FieldToJson( IASLib::CObjectNode &oNode );
        virtual BOOL        FieldFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ShapeFields( void );

            /// Each kind's default iterations.
        static int          DefaultIterations( int nKind );

    public:
                            DYNAMIC_OBJECTS( CFractal );

                            CFractal( void );
        virtual            ~CFractal( void );

            /// Sets the fractal; FALSE (with RayJson::Fail) if it's bad.
        BOOL                Set( const CVector &oCenter, int nKind, int nIterations, FLT nScale );
            /// The distance estimate at a point (in the fractal's own frame,
            /// at size 1): negative or near zero inside.
        double              Estimate( const double anPoint[ 3 ] ) const;
};

#endif // __FRACTAL_H__
