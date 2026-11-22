/*************************************************************************
*   CCombineTexture                                                      *
*                                                                        *
*   Two or more textures combined into one: multiplied together (the     *
*   default), so a large-scale map can be varied by finer solid noise,   *
*   or added with weights, so several height patterns make one bump      *
*   map. The result is then multiplied by "color" (default white).       *
*                                                                        *
*   JSON: "mode" ("multiply" or "add"), "textures" (two or more, any     *
*   types, presets included), "weights" (add only: one a texture,        *
*   default 1 each) and "color".                                         *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __COMBINETEXTURE_H__
#define __COMBINETEXTURE_H__

#include "Texture.h"

#include <memory>
#include <vector>

class CCombineTexture : public CTexture
{
    protected:
        BOOL                                    m_bAdd;
        std::vector< std::shared_ptr<const CTexture> > m_apTextures;
        std::vector<FLT>                        m_anWeights;
        CColor                                  m_oColor;

    public:
                                CCombineTexture( void );

        virtual CTexture       *Clone( void ) const { return new CCombineTexture( *this ); }
        virtual const char     *GetName( void ) const { return "combine"; }

        virtual CColor          ColorAt( const STexturePoint &oAt ) const;

        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );

        int                     GetCount( void ) const { return (int)m_apTextures.size(); }
};

#endif // __COMBINETEXTURE_H__
