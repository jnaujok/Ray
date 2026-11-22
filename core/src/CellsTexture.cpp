#include "Globals.h"

#include "CellsTexture.h"
#include "SceneJson.h"

namespace
{
    const FLT s_anGray[] =
    {
        0.0, 0.0, 0.0,
        1.0, 1.0, 1.0,
    };

    const char *const s_astrModes[] = { "flecks", "distance", "edges" };
}

CCellsTexture::CCellsTexture( void ) :
    CSolidTexture( s_anGray, 2, 0.0 ),
    m_nMode( MODE_FLECKS )
{
}

FLT CCellsTexture::Pattern( FLT nX, FLT nY, FLT nZ ) const
{
    if ( m_nTurbulence > 0.0 )
    {
            // Three offset copies of the noise move the point in x, y and z.
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
    switch ( m_nMode )
    {
        case MODE_DISTANCE:
            return nF1;
        case MODE_EDGES:
            return nF2 - nF1;
        default:
            return nCell;
    }
}

void CCellsTexture::ExtraToJson( IASLib::CObjectNode &oNode ) const
{
    oNode.put( "mode", s_astrModes[ m_nMode ] );
}

BOOL CCellsTexture::ExtraFromJson( const IASLib::CJsonNode &oNode )
{
    std::string strMode;

    if ( ! RayJson::GetString( oNode, "mode", strMode, FALSE ) )
        return FALSE;
    if ( strMode.empty() )
        return TRUE;
    for ( int nIndex = 0; nIndex < 3; nIndex++ )
    {
        if ( strMode == s_astrModes[ nIndex ] )
        {
            m_nMode = nIndex;
            return TRUE;
        }
    }

    RayJson::CWhere oWhere( "mode" );

    return RayJson::Fail( "must be \"flecks\", \"distance\" or \"edges\"" );
}

const char *const *CCellsTexture::ExtraFields( void ) const
{
    static const char *const astrFields[] = { "mode", NULL };

    return astrFields;
}
