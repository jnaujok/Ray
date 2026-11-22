/*************************************************************************
*   CFaceColorTexture                                                    *
*                                                                        *
*   See FaceColorTexture.h.                                              *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#include "Globals.h"

#include "FaceColorTexture.h"
#include "Objects.h"
#include "SceneJson.h"

CFaceColorTexture::CFaceColorTexture( void )
{
    m_oDefault.Set( 0.8, 0.8, 0.8 );
    m_bUV = FALSE;
}

CColor CFaceColorTexture::ColorAt( const STexturePoint &oAt ) const
{
    if ( m_bUV && oAt.pSurface )
    {
        CColor  oColor;
        CVector oHit( oAt.oHit );

        if ( oAt.pSurface->SurfaceImageColor( oHit, oColor ) )
            return oColor;
    }
    return oAt.bHasColor ? oAt.oColor : m_oDefault;
}

void CFaceColorTexture::ToJson( IASLib::CObjectNode &oNode ) const
{
    CColor oDefault( m_oDefault );

    RayJson::PutColor( oNode, "default", oDefault );
    if ( m_bUV )
        oNode.put( "sample", "uv" );
}

BOOL CFaceColorTexture::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "default", "sample", NULL };
    std::string              strSample = m_bUV ? "uv" : "face";

    if ( ! RayJson::OnlyFields( oNode, astrFields ) || ! RayJson::GetColor( oNode, "default", m_oDefault, FALSE ) ||
         ! RayJson::GetString( oNode, "sample", strSample, FALSE ) )
        return FALSE;
    if ( strSample != "face" && strSample != "uv" )
        return RayJson::Fail( "sample must be \"face\" (one color a face) or \"uv\" (the model's image through its coordinates)" );
    m_bUV = ( strSample == "uv" ) ? TRUE : FALSE;
    return TRUE;
}
