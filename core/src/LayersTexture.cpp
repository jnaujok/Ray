/*************************************************************************
*   CLayersTexture                                                       *
*                                                                        *
*   See LayersTexture.h.                                                 *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#include "Globals.h"

#include "LayersTexture.h"
#include "SceneJson.h"

#include <math.h>

namespace
{
        // Light gray darkening to the seams: as a color it shows the lines;
        // as a bump map's heights, the beads.
    const FLT s_anLayers[] =
    {
        0.70, 0.70, 0.70,
        1.00, 1.00, 1.00,
    };

        // The wobble's noise is this many layers across.
    const FLT WOBBLE_LAYERS = 20.0;
}

CLayersTexture::CLayersTexture( void ) :
    CSolidTexture( s_anLayers, (int)( sizeof( s_anLayers ) / sizeof( s_anLayers[ 0 ] ) / 3 ), 0.0 ),
    m_oAxis( 0.0, 0.0, 1.0 )
{
    m_nScale = 0.1;
}

FLT CLayersTexture::Bead( FLT nFraction )
{
    FLT nAcross = ( 2.0 * nFraction ) - 1.0;    // -1 at one seam, 1 at the next

    return 1.0 - ( nAcross * nAcross );
}

FLT CLayersTexture::Pattern( FLT nX, FLT nY, FLT nZ ) const
{
    CVector oAxis( m_oAxis );
    FLT     nAlong = ( nX * oAxis.GetX() ) + ( nY * oAxis.GetY() ) + ( nZ * oAxis.GetZ() );

    if ( m_nTurbulence != 0.0 )
        nAlong += m_nTurbulence * Noise( nX / WOBBLE_LAYERS, nY / WOBBLE_LAYERS, nZ / WOBBLE_LAYERS );
    return Bead( nAlong - floor( nAlong ) );
}

void CLayersTexture::ExtraToJson( IASLib::CObjectNode &oNode ) const
{
    RayJson::PutVector( oNode, "axis", m_oAxis );
}

const char *const *CLayersTexture::ExtraFields( void ) const
{
    static const char *const astrFields[] = { "axis", NULL };

    return astrFields;
}

BOOL CLayersTexture::ExtraFromJson( const IASLib::CJsonNode &oNode )
{
    if ( ! RayJson::GetVector( oNode, "axis", m_oAxis, FALSE ) )
        return FALSE;
    if ( m_oAxis.Magnitude() == 0.0 )
        return RayJson::Fail( "axis can't be [0, 0, 0]" );
    m_oAxis.Normalize();
    return TRUE;
}
