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
*   glTF 2.0, .gltf (JSON; its buffers in data: URIs or in files beside  *
*   it, which the mesh reads and passes in) or .glb (the binary          *
*   container, told apart by its "glTF" magic whatever its name):        *
*   every mesh of the default scene (or of one named node and its        *
*   children, in that node's own frame), each node's matrix or           *
*   translation, rotation and scale flattened in. Triangles, strips and  *
*   fans; points and lines are skipped. Accessors of every component     *
*   type, with byteStride, normalized integers and sparse substitution.  *
*   POSITION, NORMAL (kept if every primitive has them) and TEXCOORD_0   *
*   (likewise). Each primitive's material's pbrMetallicRoughness         *
*   baseColorFactor becomes its faces' color, times its                  *
*   baseColorTexture at the middle of each face when that image is a     *
*   PNG, JPEG or WebP (sRGB, as glTF says; the plain source, else         *
*   EXT_texture_webp's); other images (KTX2) are skipped and counted in  *
*   nSkippedImages, and the factor alone is used. Everything             *
*   else about materials, cameras, lights, skins and animations is       *
*   ignored. Draco- or meshopt-compressed models are refused. glTF is    *
*   right-handed and Ray's scenes are left-handed, so z is negated and   *
*   each face turned back round: the model isn't mirrored.               *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __MODELFILE_H__
#define __MODELFILE_H__

#include "Globals.h"
#include "Vector.h"
#include "Color.h"

#include <memory>
#include <string>
#include <utility>
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
        int                         nSkippedImages = 0;     // glTF base color images Ray couldn't read
    };

        /// The other files a .gltf names (buffers and PNG images), by the
        /// URI it gives them.
    struct SGltfFiles
    {
        std::vector< std::pair< std::string, std::shared_ptr<const std::string> > > aoFiles;
    };

        /// Reads a model from a file's contents. strName (the file's name)
        /// picks the format by its extension, ".stl" or ".obj"; any other
        /// name is told apart by the contents. FALSE, with a reason in
        /// strError, if the file can't be read as a model.
    BOOL        Read( const std::string &strData, const std::string &strName, SModel &oModel, std::string &strError );

    BOOL        ReadStl( const std::string &strData, SModel &oModel, std::string &strError );
        /// Whether a file is glTF: by the .glb magic, or a .gltf or .glb name.
    BOOL        IsGltf( const std::string &strData, const std::string &strName );
        /// Reads glTF. pFiles holds the files a .gltf names (NULL: none);
        /// strNode, if not empty, picks one node, and strMaterial, if not
        /// empty, only the primitives whose material has that name.
    BOOL        ReadGltf( const std::string &strData, const SGltfFiles *pFiles, const std::string &strNode,
                          const std::string &strMaterial, SModel &oModel, std::string &strError );
        /// The files a glTF model names besides itself: its buffers' and its
        /// PNG images' URIs (data: URIs left out). FALSE for a URI that leaves
        /// its folder, or JSON that isn't glTF.
    BOOL        GltfUris( const std::string &strData, std::vector<std::string> &astrUris, std::string &strError );
        /// A URI with its %XX escapes decoded, as a file name.
    std::string UnescapeUri( const std::string &strUri );
    BOOL        ReadObj( const std::string &strData, SModel &oModel, std::string &strError );
        /// Whether a file is a Blender .blend file: by its "BLENDER" header,
        /// or (compressed) a .blend name.
    BOOL        IsBlend( const std::string &strData, const std::string &strName );
        /// Reads every mesh object in a .blend file (or only the one named
        /// strObject), gzip- or Zstandard-compressed or not.
    BOOL        ReadBlend( const std::string &strData, const std::string &strObject, SModel &oModel, std::string &strError );
}

#endif // __MODELFILE_H__
