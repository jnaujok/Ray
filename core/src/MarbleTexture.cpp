#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "MarbleTexture.h"
#include "SceneJson.h"

#include <math.h>

namespace
{
        // White marble with thin gray veins: the vein color first, then
        // the stone, so most of the ramp is the stone.
    const FLT s_anMarble[] =
    {
        0.36, 0.36, 0.39,
        0.70, 0.70, 0.72,
        0.88, 0.87, 0.86,
        0.93, 0.92, 0.89,
        0.93, 0.92, 0.89,
        0.93, 0.92, 0.89,
        0.93, 0.92, 0.89,
        0.93, 0.92, 0.89,
    };
}

CMarbleTexture::CMarbleTexture( void ) :
    CSolidTexture( s_anMarble, (int)( sizeof( s_anMarble ) / sizeof( s_anMarble[ 0 ] ) / 3 ), 2.5 ),
    m_oAxis( 1.0, 0.0, 0.0 )
{
}

FLT CMarbleTexture::Pattern( FLT nX, FLT nY, FLT nZ ) const
{
    CVector oAxis( m_oAxis );
    FLT     nAlong = ( nX * oAxis.GetX() ) + ( nY * oAxis.GetY() ) + ( nZ * oAxis.GetZ() );

    return fabs( sin( PI * ( nAlong + ( m_nTurbulence * Turbulence( nX, nY, nZ ) ) ) ) );
}

void CMarbleTexture::ExtraToJson( IASLib::CObjectNode &oNode ) const
{
    RayJson::PutVector( oNode, "axis", m_oAxis );
}

const char *const *CMarbleTexture::ExtraFields( void ) const
{
    static const char *const astrFields[] = { "axis", NULL };

    return astrFields;
}

BOOL CMarbleTexture::ExtraFromJson( const IASLib::CJsonNode &oNode )
{
    if ( ! RayJson::GetVector( oNode, "axis", m_oAxis, FALSE ) )
        return FALSE;
    if ( m_oAxis.Magnitude() == 0.0 )
        return RayJson::Fail( "axis can't be [0, 0, 0]" );
    m_oAxis.Normalize();
    return TRUE;
}
