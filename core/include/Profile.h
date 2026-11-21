/*************************************************************************
*   Profile                                                              *
*                                                                        *
*   Flat paths for the shapes made from them: a lathe's profile, an      *
*   extrusion's outlines, a sweep's cross-section. In a scene a path is  *
*   an array of entries, each one of:                                    *
*                                                                        *
*       [x, y]                                  a straight line to x, y  *
*       { "quadratic": [ [cx, cy], [x, y] ] }   a curve to x, y pulled   *
*                                               toward the control point *
*       { "cubic": [ [c1x, c1y], [c2x, c2y], [x, y] ] }                  *
*                                               a curve to x, y with two *
*                                               control points           *
*                                                                        *
*   The first entry is where the path starts, so it must be a point. A   *
*   closed path (an outline) runs back to its start with a straight line *
*   if its last point isn't there already. With smooth, every entry must *
*   be a point and the path is a smooth curve through them all (uniform  *
*   Catmull-Rom: each piece a cubic whose tangent at a point runs        *
*   parallel to the line between its neighbors).                         *
*                                                                        *
*   Every piece is kept as a cubic (lines and quadratics raised to one), *
*   so the shapes need only one kind of curve.                           *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __PROFILE_H__
#define __PROFILE_H__

#include "Globals.h"
#include "Outline.h"

#include <vector>

namespace IASLib
{
    class CJsonNode;
    class CArrayNode;
}

namespace Profile
{
        // An entry as the scene gave it, so it is written back the same way.
    struct SEntry
    {
        int     nKind;          // 1 a point (line to), 2 quadratic, 3 cubic
        double  anPoints[ 3 ][ 2 ];
    };

        // One piece: a cubic Bezier from p[ 0 ] to p[ 3 ]. nDegree is what
        // it was given as (1 a line, 2 quadratic, 3 cubic).
    struct SPiece
    {
        int     nDegree;
        double  p[ 4 ][ 2 ];
    };

    struct SPath
    {
        std::vector<SEntry> aoEntries;
        std::vector<SPiece> aoPieces;
        bool                bClosed;
        bool                bSmooth;

                            SPath( void ) : bClosed( false ), bSmooth( false ) {}
    };

        // Reads a path (with CWhere and Fail for errors) and works out its
        // pieces. FALSE if it is bad or has fewer than nMinimumPieces.
    BOOL    Read( const IASLib::CJsonNode &oNode, bool bClosed, bool bSmooth, int nMinimumPieces, SPath &oPath );
        // Writes the entries back as they were given.
    void    Write( IASLib::CArrayNode &oArray, const SPath &oPath );
        // Works out the pieces from the entries.
    void    MakePieces( SPath &oPath );

        // A point on a piece at s (0 to 1), and the slope there.
    void    Evaluate( const SPiece &oPiece, double s, double anPoint[ 2 ] );
    void    Slope( const SPiece &oPiece, double s, double anSlope[ 2 ] );
        // The piece's coordinates as polynomials in s: x = sum anX[ i ] s^i,
        // y likewise (four coefficients each).
    void    PowerBasis( const SPiece &oPiece, double anX[ 4 ], double anY[ 4 ] );

        // The path as points no further than nTolerance from it (the
        // pieces' starts and the points between), without repeats. A
        // closed path doesn't repeat its start at the end.
    Outline::Polygon Flatten( const SPath &oPath, double nTolerance );
        // The size of the path's control points' box (its larger side).
    double  Extent( const SPath &oPath );
}

#endif // __PROFILE_H__
