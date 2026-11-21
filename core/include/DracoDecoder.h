/*************************************************************************
*   DracoDecoder                                                         *
*                                                                        *
*   Decodes Draco-compressed triangle meshes (bitstream version 2.2, the *
*   one KHR_draco_mesh_compression glTF files carry), for the glTF       *
*   reader. Written from the Draco bitstream specification and checked   *
*   against what Google's decoder produces:                              *
*                                                                        *
*   - connectivity: sequential (raw or entropy-coded indices) or         *
*     EdgeBreaker, with the standard or the valence traversal,           *
*     topology splits, and attribute seams (UV and normal seams);        *
*   - attributes: the depth-first and the prediction-degree traversals,  *
*     integer, quantized and octahedral-normal values, raw or rANS       *
*     coded (raw and tagged symbols);                                    *
*   - prediction: difference, parallelogram, constrained                 *
*     multi-parallelogram, portable texture coordinates and geometric    *
*     normals, with the wrap and the two octahedron transforms.          *
*                                                                        *
*   Point clouds, the deprecated prediction schemes and bitstreams older *
*   than 2.2 are refused with a reason.                                  *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __DRACODECODER_H__
#define __DRACODECODER_H__

#include <cstddef>
#include <string>
#include <vector>

namespace Draco
{
        // Draco's attribute types.
    enum EAttributeType { ATTRIBUTE_POSITION = 0, ATTRIBUTE_NORMAL = 1, ATTRIBUTE_COLOR = 2,
                          ATTRIBUTE_TEXCOORD = 3, ATTRIBUTE_GENERIC = 4 };

    struct SAttribute
    {
        int                 nType = 0;          // an EAttributeType
        int                 nUniqueId = 0;      // the id glTF's extension names it by
        int                 nComponents = 0;
        std::vector<float>  afValues;           // nComponents a value
        std::vector<int>    anCornerValue;      // the value at each corner (three a face)
    };

    struct SMesh
    {
        int                     nFaces = 0;
        std::vector<SAttribute> aoAttributes;

            /// The attribute with that unique id, or NULL.
        const SAttribute       *ByUniqueId( int nUniqueId ) const;
    };

        /// Decodes a Draco mesh. FALSE, with why in strError, if it isn't one
        /// Ray can decode or it is damaged.
    bool    Decode( const unsigned char *pData, size_t nSize, SMesh &oMesh, std::string &strError );
}

#endif // __DRACODECODER_H__
