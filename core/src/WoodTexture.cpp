#include "Globals.h"

#include "WoodTexture.h"
#include "SceneJson.h"

#include <math.h>

namespace
{
        // Pine: pale early wood darkening to a thin late-wood ring.
    const FLT s_anPine[] =
    {
        0.85, 0.66, 0.42,
        0.80, 0.60, 0.37,
        0.72, 0.51, 0.30,
        0.47, 0.30, 0.16,
    };
}

CWoodTexture::CWoodTexture( void ) :
    CSolidTexture( s_anPine, (int)( sizeof( s_anPine ) / sizeof( s_anPine[ 0 ] ) / 3 ), 0.3 ),
    m_oAxis( 0.0, 1.0, 0.0 )
{
}

FLT CWoodTexture::Pattern( FLT nX, FLT nY, FLT nZ ) const
{
    CVector oAxis( m_oAxis );
    FLT     nAlong = ( nX * oAxis.GetX() ) + ( nY * oAxis.GetY() ) + ( nZ * oAxis.GetZ() );
    FLT     nAcrossX = nX - ( nAlong * oAxis.GetX() );
    FLT     nAcrossY = nY - ( nAlong * oAxis.GetY() );
    FLT     nAcrossZ = nZ - ( nAlong * oAxis.GetZ() );
        // Noise stretched along the grain, so the rings wobble slowly up the
        // trunk and quickly around it.
    FLT     nRing = SQRT( ( nAcrossX * nAcrossX ) + ( nAcrossY * nAcrossY ) + ( nAcrossZ * nAcrossZ ) ) +
                    ( m_nTurbulence * Turbulence( nX - ( 0.8 * nAlong * oAxis.GetX() ),
                                                  nY - ( 0.8 * nAlong * oAxis.GetY() ),
                                                  nZ - ( 0.8 * nAlong * oAxis.GetZ() ) ) );

    return nRing - floor( nRing );
}

void CWoodTexture::ExtraToJson( IASLib::CObjectNode &oNode ) const
{
    RayJson::PutVector( oNode, "axis", m_oAxis );
}

const char *const *CWoodTexture::ExtraFields( void ) const
{
    static const char *const astrFields[] = { "axis", NULL };

    return astrFields;
}

BOOL CWoodTexture::ExtraFromJson( const IASLib::CJsonNode &oNode )
{
    if ( ! RayJson::GetVector( oNode, "axis", m_oAxis, FALSE ) )
        return FALSE;
    if ( m_oAxis.Magnitude() == 0.0 )
        return RayJson::Fail( "axis can't be [0, 0, 0]" );
    m_oAxis.Normalize();
    return TRUE;
}
