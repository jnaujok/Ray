/*************************************************************************
*   CFaceColorTexture                                                    *
*                                                                        *
*   The colors a model file gives its faces (a colored STL), as a        *
*   texture: each face shows its own color, and a face the file gives no *
*   color, or a surface that isn't from a colored model, shows "default" *
*   (default [0.8, 0.8, 0.8]). Like any texture it goes on the materials *
*   that should show it, typically the ambient and diffuse ones, so a    *
*   specular highlight can stay white.                                   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __FACECOLORTEXTURE_H__
#define __FACECOLORTEXTURE_H__

#include "Texture.h"

class CFaceColorTexture : public CTexture
{
    protected:
        CColor                  m_oDefault;

    public:
                                CFaceColorTexture( void );

        virtual CTexture       *Clone( void ) const { return new CFaceColorTexture( *this ); }
        virtual const char     *GetName( void ) const { return "faceColors"; }

        virtual CColor          ColorAt( const STexturePoint &oAt ) const;
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __FACECOLORTEXTURE_H__
