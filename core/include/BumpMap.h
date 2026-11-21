/*************************************************************************
*   CBumpMap                                                             *
*                                                                        *
*   Bumps on a surface that aren't in its shape: the normal used for     *
*   shading is tilted at each point, so light and highlights fall as if  *
*   the surface were dented, grooved or rough, while rays still hit the  *
*   smooth shape (and its outline stays smooth).                         *
*                                                                        *
*   A bump map belongs to an object (its "bump" field), not to one of    *
*   its materials, so every material on the object sees the same bumps:  *
*   the diffuse shading, the highlight, and the reflected and refracted  *
*   rays all follow the tilted normal. A mesh's faces share the mesh's.  *
*                                                                        *
*   Two kinds, set by "mode":                                            *
*                                                                        *
*   - "height" (the default): any texture is read as a height field. A   *
*     point's height is the average of the texture's red, green and      *
*     blue there, times "depth" (in scene units; default 0.05). The      *
*     normal tilts down the slope of the heights, measured by looking    *
*     the texture up a "step" (default 0.001 scene units) either side    *
*     of the point along the surface. Solid textures (marble, wood,      *
*     noise) need no surface coordinates, so they bump any object.       *
*                                                                        *
*   - "normal": a tangent-space normal map, usually an image. A pixel's  *
*     red, green and blue, each 0 to 1, are the normal's components      *
*     (2 c - 1) along the surface's u direction, its v direction, and    *
*     out of the surface: the common OpenGL convention, where flat is    *
*     (0.5, 0.5, 1) and green points toward increasing v (up the image). *
*     "strength" (default 1) scales the tilt. The u and v directions     *
*     come from the object's surface coordinates, so the object needs    *
*     them (spheres, planes, boxes, cylinders, cones, and triangles and  *
*     meshes with uvs or a UV map).                                      *
*                                                                        *
*   A tilted normal that would face away from the ray (at the edge of a  *
*   sphere, say) is passed over for the smooth one, so a surface never   *
*   looks lit from behind.                                               *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __BUMPMAP_H__
#define __BUMPMAP_H__

#include "Globals.h"
#include "Vector.h"
#include "Maps.h"

#include <memory>

class CObjects;
class CRay;
class CTexture;

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

class CBumpMap
{
    public:
        enum EMode { MODE_HEIGHT, MODE_NORMAL };

    protected:
        std::unique_ptr<CTexture>   m_pTexture;
        EMode                       m_eMode;
        CScalarMap                  m_nDepth;       // height mode: scene units at a texture value of 1 (or a texture)
        CScalarMap                  m_nStrength;    // normal mode: how far the map tilts the normal (or a texture)
        FLT                         m_nStep;        // how far apart the texture is looked up

    public:
                                CBumpMap( void );
                                CBumpMap( const CBumpMap &oSource );
                               ~CBumpMap( void );

        EMode                   GetMode( void ) const { return m_eMode; }

            /// Tilts oNormal (the smooth surface's normal at oPoint on
            /// oObject, hit by oRay) by the map. oNormal comes back a unit
            /// vector, keeping its inside flag; it is left as it was if the
            /// map can't be applied there (no surface coordinates, or the
            /// tilted normal faces away from the ray).
        void                    Perturb( CObjects &oObject, const CVector &oPoint, CVector &oNormal, CRay &oRay ) const;

        void                    ToJson( IASLib::CObjectNode &oNode ) const;
        BOOL                    FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __BUMPMAP_H__
