/*************************************************************************
*   CTexture                                                             *
*                                                                        *
*   A pattern laid over a surface: given where a ray hit, the color      *
*   there. A material with a texture uses it in place of its own color.  *
*                                                                        *
*   A texture is told the hit point in the scene and, where the object   *
*   has them, its surface coordinates (u, v): longitude and latitude on  *
*   a sphere, distances along two axes across a plane, and corner or     *
*   vertex coordinates on triangles and meshes. Solid textures use the   *
*   point itself, so they need no surface coordinates.                   *
*                                                                        *
*   To add a texture, derive from this class and add it to the table in  *
*   Texture.cpp so it can be named in a scene.                           *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "Globals.h"
#include "Color.h"
#include "Vector.h"

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

    // Where on a surface a texture is looked up.
struct STexturePoint
{
    CVector             oPoint;         // the hit point, in the object's texture space (usually the scene's)
    FLT                 nU;             // surface coordinates, if bHasUV
    FLT                 nV;
    BOOL                bHasUV;
    CColor              oColor;         // the surface's own color (a colored model's face), if bHasColor
    BOOL                bHasColor;
};

class CTexture
{
    public:
        struct SType
        {
            const char         *strName;
            const char         *strDescription;
            CTexture         *(*pfnCreate)( void );
        };

        virtual            ~CTexture( void ) {}

        virtual CTexture       *Clone( void ) const = 0;
        virtual const char     *GetName( void ) const = 0;

            /// The color at a point on the surface.
        virtual CColor          ColorAt( const STexturePoint &oAt ) const = 0;

            /// The texture's settings as JSON (oNode already holds "type").
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const = 0;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode ) = 0;

        static CTexture        *Create( const char *strName );
        static const SType     *GetTypes( int &nCount );
};

#endif // __TEXTURE_H__
