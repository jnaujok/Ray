/*************************************************************************
*   CFieldShape                                                          *
*                                                                        *
*   The base of the shapes defined by a number at every point: an        *
*   implicit surface (a formula), a blob (metaballs) and a fractal (a    *
*   distance estimate). The surface is where the number is zero; inside  *
*   it is negative.                                                      *
*                                                                        *
*   This class adds what the three share to a CLocalShape: a uniform     *
*   "scale" (the shape is worked out at size 1 and scaled), surface      *
*   coordinates projected onto a sphere around the center (u the angle   *
*   around +y, v from the bottom, 0, to the top, 1), and triangles for   *
*   the editor's preview by marching tetrahedra through the bounds.      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __FIELDSHAPE_H__
#define __FIELDSHAPE_H__

#include "LocalShape.h"

class CFieldShape : public CLocalShape
{
    protected:
        FLT                 m_nScale;

            // What each shape answers, at size 1 (before "scale").
        virtual BOOL        PrepareField( void ) = 0;
            /// The number at a point: negative inside, zero on the surface.
        virtual double      Field( const double anPoint[ 3 ] ) const = 0;
            /// The nearest place in (nLow, nHigh) along the unit direction
            /// where the ray meets the surface.
        virtual BOOL        FieldHit( const double anOrigin[ 3 ], const double anDirection[ 3 ], double nLow, double nHigh,
                                      double &nT ) const = 0;
            /// The outward normal (need not be a unit vector).
        virtual void        FieldNormal( const double anPoint[ 3 ], double anNormal[ 3 ] ) const = 0;
            /// The box the shape stays inside.
        virtual void        FieldBounds( double anLow[ 3 ], double anHigh[ 3 ] ) const = 0;
        virtual void        FieldToJson( IASLib::CObjectNode &oNode ) = 0;
        virtual BOOL        FieldFromJson( const IASLib::CJsonNode &oNode ) = 0;

            // CLocalShape, worked out through the above.
        virtual BOOL        PrepareShape( void );
        virtual BOOL        LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT );
        virtual void        LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] );
        virtual void        LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV );
        virtual void        LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] );
        virtual void        ShapeToJson( IASLib::CObjectNode &oNode );
        virtual BOOL        ShapeFromJson( const IASLib::CJsonNode &oNode );
        virtual void        LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals );

            /// Where a ray's unit-direction line crosses the box: FALSE if
            /// it misses it or the crossing is outside (nLow, nHigh).
        static BOOL         ClipToBox( const double anOrigin[ 3 ], const double anDirection[ 3 ], const double anLow[ 3 ],
                                       const double anHigh[ 3 ], double &nLow, double &nHigh );
            /// The slope of the field by central differences of step nStep.
        void                FieldGradient( const double anPoint[ 3 ], double nStep, double anGradient[ 3 ] ) const;

    public:
                            CFieldShape( void );
        virtual            ~CFieldShape( void );

            /// The number at a point in the shape's own frame, scaled, for
            /// tests: negative inside.
        double              FieldAt( const double anPoint[ 3 ] ) const;
};

#endif // __FIELDSHAPE_H__
