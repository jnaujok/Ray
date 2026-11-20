/*************************************************************************
*   ModelFile                                                            *
*                                                                        *
*   Reads the triangle models a mesh can load from a file:               *
*                                                                        *
*   STL, binary or ASCII. A binary file is recognized by its size (an    *
*   80-byte header, a count, and 50 bytes a triangle), since many begin  *
*   "solid" just as ASCII ones do. Corners at exactly the same place are *
*   joined into one vertex, so a smooth mesh can blend across faces. The *
*   facet normals are ignored: as in the STL standard, each face's       *
*   corners run counterclockwise seen from outside, which Ray takes as   *
*   the front.                                                           *
*                                                                        *
*   Colored binary STL, in either of the two informal conventions:       *
*     - Materialise Magics: the header holds "COLOR=" and four bytes, the *
*       red, green, blue and alpha of the whole model. A face whose       *
*       attribute has bit 15 clear has its own color: red in bits 0-4,    *
*       green 5-9, blue 10-14 (each 0 to 31); with bit 15 set it takes    *
*       the model's color.                                                *
*     - VisCAM and SolidView (when the header has no "COLOR="): a face    *
*       whose attribute has bit 15 set has a color, blue in bits 0-4,     *
*       green 5-9, red 10-14; with it clear the face has none.           *
*                                                                        *
*   OBJ: "v" (x, y, z; a fourth number or colors after it are ignored),  *
*   "vt" (u, v), "vn" and "f" lines. A face may have any number of       *
*   corners (it is split into a fan of triangles from its first), each   *
*   "v", "v/vt", "v//vn" or "v/vt/vn", counted from 1 or, negative, back *
*   from the last one read. Each different combination of position,      *
*   coordinates and normal becomes a vertex. Surface coordinates are     *
*   kept only if every corner has them, and normals likewise. Objects,   *
*   groups, smoothing groups and materials ("o", "g", "s", "usemtl",     *
*   "mtllib") are ignored: the mesh's own materials apply to all of it.  *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __MODELFILE_H__
#define __MODELFILE_H__

#include "Globals.h"
#include "Vector.h"
#include "Color.h"

#include <string>
#include <vector>

namespace ModelFile
{
    struct SModel
    {
        std::vector<CVector>        aoVertices;
        std::vector<int>            anFaces;        // three vertex indices a face
        std::vector<CVector>        aoNormals;      // one a vertex, or empty
        std::vector<FLT>            anUVs;          // u and v a vertex, or empty
        std::vector<CColor>         aoFaceColors;   // one a face, or empty if the file has no colors
        std::vector<unsigned char>  abHasColor;     // whether each face has its color (with aoFaceColors)
    };

        /// Reads a model from a file's contents. strName (the file's name)
        /// picks the format by its extension, ".stl" or ".obj"; any other
        /// name is told apart by the contents. FALSE, with a reason in
        /// strError, if the file can't be read as a model.
    BOOL        Read( const std::string &strData, const std::string &strName, SModel &oModel, std::string &strError );

    BOOL        ReadStl( const std::string &strData, SModel &oModel, std::string &strError );
    BOOL        ReadObj( const std::string &strData, SModel &oModel, std::string &strError );
}

#endif // __MODELFILE_H__
