/*************************************************************************
*   CFaceColorTexture                                                    *
*                                                                        *
*   See FaceColorTexture.h.                                              *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "FaceColorTexture.h"
#include "SceneJson.h"

CFaceColorTexture::CFaceColorTexture( void )
{
    m_oDefault.Set( 0.8, 0.8, 0.8 );
}

CColor CFaceColorTexture::ColorAt( const STexturePoint &oAt ) const
{
    return oAt.bHasColor ? oAt.oColor : m_oDefault;
}

void CFaceColorTexture::ToJson( IASLib::CObjectNode &oNode ) const
{
    CColor oDefault( m_oDefault );

    RayJson::PutColor( oNode, "default", oDefault );
}

BOOL CFaceColorTexture::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "default", NULL };

    return RayJson::OnlyFields( oNode, astrFields ) &&
           RayJson::GetColor( oNode, "default", m_oDefault, FALSE );
}
