/*************************************************************************
*   CCheckerTexture                                                      *
*                                                                        *
*   A checkerboard of two colors. On the surface, squares "size" across  *
*   in surface coordinates; "solid", cubes "size" across in the scene,   *
*   which needs no surface coordinates and cuts through objects as if    *
*   they were carved from a checkered block.                             *
*                                                                        *
*   JSON settings: "colors" (two colors; default black and white),       *
*   "size" (default 1), "solid" (default false).                         *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __CHECKERTEXTURE_H__
#define __CHECKERTEXTURE_H__

#include "Texture.h"

class CCheckerTexture : public CTexture
{
    protected:
        CColor                  m_aoColors[ 2 ];
        FLT                     m_nSize;
        BOOL                    m_bSolid;

    public:
                                CCheckerTexture( void );

        virtual CTexture       *Clone( void ) const { return new CCheckerTexture( *this ); }
        virtual const char     *GetName( void ) const { return "checker"; }

        virtual CColor          ColorAt( const STexturePoint &oAt ) const;
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __CHECKERTEXTURE_H__
