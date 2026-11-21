/*************************************************************************
*   Outline                                                              *
*                                                                        *
*   Flat shapes made of closed outlines, as a font's glyphs are: curves  *
*   flattened into polygons, sorted into filled areas and their holes,   *
*   and cut into triangles.                                              *
*                                                                        *
*   Which outlines are holes is worked out from how they nest, not which *
*   way they run: an outline inside an even number of others is filled, *
*   inside an odd number a hole, so fonts that run their outlines either *
*   way come out right. Each hole is joined to the outline around it by  *
*   a cut to a vertex it can see (Eberly's method), and the one polygon  *
*   that makes is cut into triangles by clipping ears.                   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __OUTLINE_H__
#define __OUTLINE_H__

#include "FontFile.h"

#include <string>
#include <vector>

namespace Outline
{
    struct SPoint
    {
        double  x;
        double  y;
    };
    typedef std::vector<SPoint> Polygon;

        // A filled flat shape: its points, triangles (three indices each,
        // counterclockwise), and its outlines as loops of indices, each run
        // so the filled side is on its left (the outside of a filled area
        // counterclockwise, a hole clockwise).
    struct SShape
    {
        std::vector<SPoint>             aoPoints;
        std::vector<int>                anTriangles;
        std::vector< std::vector<int> > aanLoops;
    };

        // A font contour (points on and off the curve) as a polygon, each
        // curve cut into straight pieces until none strays more than
        // nTolerance from it. Repeated and in-line points are dropped.
    Polygon Flatten( const CFontFile::Contour &oContour, double nTolerance );

        // Twice the polygon's signed area: positive if it runs
        // counterclockwise (with y up).
    double  SignedArea2( const Polygon &oPolygon );

        // Fills the outlines (holes worked out from nesting) and cuts them
        // into triangles. FALSE, with a reason, if an outline can't be cut
        // (it crosses itself badly).
    bool    Fill( const std::vector<Polygon> &aoOutlines, SShape &oShape, std::string &strError );

        // The loops of a filled shape moved nInset toward the filled side
        // (each corner along the bisector of its edges' normals, at most
        // three times nInset), as polygons run the same way.
    std::vector<Polygon> Inset( const SShape &oShape, double nInset );
}

#endif // __OUTLINE_H__
