#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "CausticsTexture.h"
#include "SceneJson.h"

#include <math.h>
#include <algorithm>

namespace
{
    const FLT s_anCaustic[] =
    {
        0.35, 0.40, 0.42,
        1.40, 1.50, 1.45,
    };
}

CCausticsTexture::CCausticsTexture( void ) :
    CSolidTexture( s_anCaustic, 2, 0.6 ),
    m_nWidth( 0.12 ),
    m_nLayers( 2 )
{
}

FLT CCausticsTexture::Lines( FLT nX, FLT nY, FLT nZ ) const
{
    if ( m_nTurbulence > 0.0 )
    {
        FLT nMoveX = Noise( nX + 31.7, nY + 12.3, nZ + 5.9 );
        FLT nMoveY = Noise( nX - 17.1, nY + 41.9, nZ - 23.3 );
        FLT nMoveZ = Noise( nX + 7.7, nY - 29.5, nZ + 37.1 );

        nX += m_nTurbulence * nMoveX;
        nY += m_nTurbulence * nMoveY;
        nZ += m_nTurbulence * nMoveZ;
    }

    FLT nF1;
    FLT nF2;
    FLT nCell;

    Cells( nX, nY, nZ, nF1, nF2, nCell );

    FLT nGap = ( nF2 - nF1 ) / m_nWidth;

    if ( nGap >= 1.0 )
        return 0.0;
        // Smoothstep down from the line to the width.
    FLT nT = 1.0 - nGap;

    return nT * nT * ( 3.0 - ( 2.0 * nT ) );
}

FLT CCausticsTexture::Pattern( FLT nX, FLT nY, FLT nZ ) const
{
    FLT nValue = Lines( nX, nY, nZ );

        // Each further layer is turned (about 37 degrees about a slanted
        // axis, roughly), finer and fainter, so the web never looks tiled.
    if ( m_nLayers >= 2 )
    {
        FLT nU = ( 0.80 * nX ) - ( 0.60 * nZ ) + 13.1;
        FLT nV = nY + 7.3;
        FLT nW = ( 0.60 * nX ) + ( 0.80 * nZ ) - 4.9;

        nValue = std::max( nValue, 0.6 * Lines( nU * 1.7, nV * 1.7, nW * 1.7 ) );
    }
    if ( m_nLayers >= 3 )
    {
        FLT nU = ( 0.28 * nX ) + ( 0.96 * nZ ) - 21.7;
        FLT nV = nY - 3.1;
        FLT nW = ( -0.96 * nX ) + ( 0.28 * nZ ) + 9.4;

        nValue = std::max( nValue, 0.35 * Lines( nU * 2.9, nV * 2.9, nW * 2.9 ) );
    }
    return nValue;
}

void CCausticsTexture::ExtraToJson( IASLib::CObjectNode &oNode ) const
{
    RayJson::PutNumber( oNode, "width", m_nWidth );
    oNode.put( "layers", m_nLayers );
}

BOOL CCausticsTexture::ExtraFromJson( const IASLib::CJsonNode &oNode )
{
    if ( ! RayJson::GetNumber( oNode, "width", m_nWidth, FALSE ) ||
         ! RayJson::GetInt( oNode, "layers", m_nLayers, 1, 3, FALSE ) )
        return FALSE;
    if ( ! ( m_nWidth > 0.0 && m_nWidth <= 1.0 ) )
    {
        RayJson::CWhere oWhere( "width" );

        return RayJson::Fail( "must be more than 0 and at most 1" );
    }
    return TRUE;
}

const char *const *CCausticsTexture::ExtraFields( void ) const
{
    static const char *const astrFields[] = { "width", "layers", NULL };

    return astrFields;
}
