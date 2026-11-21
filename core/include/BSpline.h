/*************************************************************************
*   CBSpline                                                             *
*                                                                        *
*   A B-spline surface: a net of control points ("points", rows along v, *
*   each row along u), a degree each way ("degree", default [3, 3]) and  *
*   a knot vector each way ("knotsU", "knotsV"; default clamped and      *
*   evenly spaced). A point may carry a weight ([x, y, z, w]), which     *
*   makes it a NURBS surface, so circles, spheres and tori come out      *
*   exact.                                                               *
*                                                                        *
*   It is traced as Bezier patches: knots are inserted (Boehm's          *
*   algorithm, in homogeneous coordinates so weights carry through)      *
*   until every knot in the surface's range repeats as often as the      *
*   degree, which cuts it into rational Bezier patches that are the same *
*   surface exactly, and those are traced as CBezier traces any patch.   *
*   Surface coordinates run from 0 to 1 across the whole surface (the    *
*   knot range), not patch by patch.                                     *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __BSPLINE_H__
#define __BSPLINE_H__

#include "Bezier.h"

class CBSpline : public CBezier
{
    protected:
        int                     m_nDegreeU;
        int                     m_nDegreeV;
        int                     m_nCountU;      // points along u (in a row)
        int                     m_nCountV;      // rows
        std::vector<double>     m_anNet;        // homogeneous, rows along v, u fastest
        std::vector<double>     m_anKnotsU;
        std::vector<double>     m_anKnotsV;
        BOOL                    m_bOwnKnotsU;   // the scene gave them
        BOOL                    m_bOwnKnotsV;

        virtual void        ShapeToJson( IASLib::CObjectNode &oNode );
        virtual BOOL        ShapeFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ShapeFields( void );

    public:
                            DYNAMIC_OBJECTS( CBSpline );

                            CBSpline( void );
        virtual            ~CBSpline( void );

            /// Sets the surface directly: nCountU by nCountV homogeneous points
            /// (x w, y w, z w, w; rows along v), the degrees, and the knot
            /// vectors (empty for clamped and even). FALSE (with
            /// RayJson::Fail) if they don't fit together.
        BOOL                SetSurface( int nDegreeU, int nDegreeV, int nCountU, int nCountV, const std::vector<double> &anNet,
                                        const std::vector<double> &anKnotsU, const std::vector<double> &anKnotsV );

            /// A point of the surface at knot parameters (u, v), straight
            /// from the B-spline (de Boor's algorithm), for checking the
            /// patches against.
        void                SurfacePoint( double u, double v, double anPoint[ 3 ] ) const;
            /// The knot range each way: [u start, u end, v start, v end].
        void                Range( double anRange[ 4 ] ) const;

            /// Clamped, evenly spaced knots for nCount points of degree
            /// nDegree.
        static std::vector<double> ClampedKnots( int nCount, int nDegree );
            /// Inserts knots in a curve of homogeneous points until every
            /// knot in its range repeats nDegree times; FALSE if the knots
            /// are bad.
        static BOOL         ToBezier( std::vector<double> &anCurve, std::vector<double> &anKnots, int nDegree );

    private:
        BOOL                BuildPatches( void );
};

#endif // __BSPLINE_H__
