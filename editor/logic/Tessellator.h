/*************************************************************************
*   Tessellator                                                          *
*                                                                        *
*   Turns the scene's objects into triangles with normals, and its       *
*   lights and camera into line drawings, for the scene editor's shaded  *
*   OpenGL view. Shapes are read from each object's JSON (the same text  *
*   the scene file holds), meshes from the mesh's own geometry. This is  *
*   only a preview: textures show as one color, and a csg shows its      *
*   parts, faintly, rather than the solid they make.                     *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __TESSELLATOR_H__
#define __TESSELLATOR_H__

#include "EditorMath.h"

#include <string>
#include <vector>

namespace IASLib
{
    class CJsonNode;
}

class CObjects;
class CScene;

namespace RayEditor
{
        // One object's triangles: three positions and three normals (x, y,
        // z each) a corner, nine numbers a triangle.
    struct SDrawMesh
    {
        int                 nObject = -1;       // the scene object it draws
        std::vector<float>  anPositions;
        std::vector<float>  anNormals;
        float               anColor[ 4 ] = { 0.7f, 0.7f, 0.7f, 1.0f };
        bool                bGhost = false;     // a csg's part: drawn faint
        bool                bPlane = false;     // an infinite plane, drawn as a large square

        size_t              TriangleCount( void ) const { return anPositions.size() / 9; }
    };

        // A line drawing: pairs of points, six numbers a line.
    struct SDrawLines
    {
        int                 nLight = -1;        // the light it draws, or -1 for the camera
        std::vector<float>  anPoints;
        float               anColor[ 4 ] = { 1.0f, 0.9f, 0.3f, 1.0f };

        size_t              LineCount( void ) const { return anPoints.size() / 6; }
    };

        // How finely curved shapes are cut: segments around a circle.
    const int DEFAULT_SEGMENTS = 32;

        /// The color an object shows in the preview, from its JSON: its
        /// diffuse (or first) material's color, or its texture's colors
        /// averaged, and translucent if it is mostly transparent.
    void        PreviewColor( const IASLib::CJsonNode &oObject, float anColor[ 4 ] );

        /// Adds an object's triangles to aoOut (more than one for a csg's
        /// parts). pObject is the scene's object, for a mesh's geometry; nPlane
        /// is how far an infinite plane is drawn from its position. FALSE if
        /// the object's shape isn't known (nothing is added).
    bool        TessellateObject( const IASLib::CJsonNode &oObject, CObjects *pObject, int nObject, double nPlane,
                                  std::vector<SDrawMesh> &aoOut, int nSegments = DEFAULT_SEGMENTS );

        /// A light's icon, from its JSON: a star for a point, a ring for a
        /// sphere light, the panel for a rectangle light, the cone for a spot,
        /// arrows for the sun (drawn about oCenter, nSize long).
    bool        LightIcon( const IASLib::CJsonNode &oLight, int nLight, const SVec3 &oCenter, double nSize, SDrawLines &oOut );

        /// The scene camera's icon: its frustum to nLength in front of it,
        /// for an image nAspect wide over its height.
    bool        CameraIcon( const IASLib::CJsonNode &oCamera, double nAspect, double nLength, SDrawLines &oOut );

        /// The whole scene: every object it can draw, every light and the
        /// camera, and the box around the objects that have one.
    struct SSceneDrawing
    {
        std::vector<SDrawMesh>  aoMeshes;
        std::vector<SDrawLines> aoLines;
        SVec3                   oMinimum;
        SVec3                   oMaximum;
        bool                    bHasBounds = false;
    };
    void        TessellateScene( CScene &oScene, SSceneDrawing &oOut, int nSegments = DEFAULT_SEGMENTS );

        /// A vector from a JSON array of three numbers (FALSE if it isn't one).
    bool        JsonVector( const IASLib::CJsonNode *pNode, SVec3 &oOut );
}

#endif // __TESSELLATOR_H__
