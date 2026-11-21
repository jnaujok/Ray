/*************************************************************************
*   Extruder                                                             *
*                                                                        *
*   Builds closed solids from flat shapes (see Outline.h): the shape's   *
*   front face at z = 0 facing -z, its back face at the depth facing +z, *
*   and the sides between, smooth around curves and sharp at corners     *
*   (where the outline turns more than 35 degrees). Text (Text.h) and    *
*   extrusions (Extrusion.h) are both made this way.                     *
*                                                                        *
*   A bevel cuts the front (or back) edges at 45 degrees, as deep as it  *
*   is wide: the face is the outline moved in by the bevel, and a        *
*   leaning band joins it to the outline. A stroke too thin for the      *
*   bevel is left square.                                                *
*                                                                        *
*   A taper scales the shape toward the back (1 keeps it the same, 0.5   *
*   halves it) and a twist turns it, both about the shape's own origin   *
*   (x = y = 0); the sides of a twisted shape are cut into slices along  *
*   the depth, one each 2 degrees of twist.                              *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __EXTRUDER_H__
#define __EXTRUDER_H__

#include "Outline.h"
#include "Vector.h"

#include <vector>

namespace Extruder
{
        // Collects a mesh: vertices with normals and surface coordinates,
        // and faces, each turned so it faces the way its normal says.
    struct SBuilder
    {
        std::vector<CVector>    aoVertices;
        std::vector<CVector>    aoNormals;
        std::vector<FLT>        anUVs;
        std::vector<int>        anFaces;
        FLT                     nPerUnit;       // u and v are x and y times this

                                SBuilder( void ) : nPerUnit( 1.0 ) {}

            // A vertex at x, y, z with the normal (made unit length); its
            // surface coordinates are x and y times nPerUnit.
        int     Add( double x, double y, double z, double nx, double ny, double nz );
            // The same, with surface coordinates given.
        int     AddUV( double x, double y, double z, double nx, double ny, double nz, double u, double v );
            // A face, its corners swapped if it would face away from n.
        void    Face( int a, int b, int c, double nx, double ny, double nz );
        void    Quad( int a, int b, int c, int d, double nx, double ny, double nz );
    };

        // Twice the area of a filled shape's triangles.
    double  TriangleArea2( const Outline::SShape &oShape );

        // The filled shape extruded nDepth back along +z, its front edges
        // beveled nBevel deep and its back edges nBackBevel deep.
    void    Extrude( SBuilder &oOut, const Outline::SShape &oShape, double nDepth, double nBevel, double nBackBevel = 0.0 );

        // The filled shape extruded nDepth back, scaled by nTaper at the
        // back and turned nTwist radians (counterclockwise seen from the
        // front, looking along +z) by then, about the shape's origin.
    void    ExtrudeTapered( SBuilder &oOut, const Outline::SShape &oShape, double nDepth, double nTaper, double nTwist );

        // Turns every vertex and normal about x, then y, then z by the
        // angles in degrees (as a box's rotation), then moves it by oMove.
    void    Place( SBuilder &oOut, CVector oRotationDegrees, CVector oMove );
}

#endif // __EXTRUDER_H__
