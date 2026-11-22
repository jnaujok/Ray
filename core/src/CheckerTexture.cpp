#include "Globals.h"

#include "CheckerTexture.h"
#include "SceneJson.h"

#include <math.h>

    // Solid checks are moved this far (in sizes) off the scene's grid, so a
    // surface lying exactly on a boundary, such as a floor at y = 0, doesn't
    // flicker between two cubes with rounding.
#define SOLID_SHIFT     1e-6

CCheckerTexture::CCheckerTexture( void ) :
    m_nSize( 1.0 ),
    m_bSolid( FALSE )
{
    m_aoColors[ 0 ].Set( 0.0, 0.0, 0.0 );
    m_aoColors[ 1 ].Set( 1.0, 1.0, 1.0 );
}

CColor CCheckerTexture::ColorAt( const STexturePoint &oAt ) const
{
    long long nSum;

    if ( m_bSolid || ! oAt.bHasUV )
    {
        CVector oPoint( oAt.oPoint );

        nSum = (long long)floor( (double)( oPoint.GetX() / m_nSize ) + SOLID_SHIFT ) +
               (long long)floor( (double)( oPoint.GetY() / m_nSize ) + SOLID_SHIFT ) +
               (long long)floor( (double)( oPoint.GetZ() / m_nSize ) + SOLID_SHIFT );
    }
    else
        nSum = (long long)floor( (double)( oAt.nU / m_nSize ) ) + (long long)floor( (double)( oAt.nV / m_nSize ) );

    return m_aoColors[ ( nSum & 1 ) ? 1 : 0 ];
}

void CCheckerTexture::ToJson( IASLib::CObjectNode &oNode ) const
{
    IASLib::CArrayNode *pColors = oNode.putArray( "colors" );

    for ( int nIndex = 0; nIndex < 2; nIndex++ )
    {
        CColor oColor( m_aoColors[ nIndex ] );
        pColors->addArray()->add( (double)oColor.GetR() )->add( (double)oColor.GetG() )->add( (double)oColor.GetB() );
    }
    RayJson::PutNumber( oNode, "size", m_nSize );
    RayJson::PutBool( oNode, "solid", m_bSolid );
}

BOOL CCheckerTexture::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "colors", "size", "solid", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetNumber( oNode, "size", m_nSize, FALSE ) ||
         ! RayJson::GetBool( oNode, "solid", m_bSolid, FALSE ) )
        return FALSE;
    if ( ! ( m_nSize > 0.0 ) )
        return RayJson::Fail( "size must be positive" );

    const IASLib::CJsonNode *pColors = oNode.get( "colors" );

    if ( pColors )
    {
        RayJson::CWhere oWhere( "colors" );

        if ( ! pColors->isArray() || pColors->size() != 2 )
            return RayJson::Fail( "must be an array of two colors" );
        for ( int nIndex = 0; nIndex < 2; nIndex++ )
        {
            RayJson::CWhere oElement( "", nIndex );
            CVector         oColor;

            if ( ! RayJson::ToVector( *pColors->get( nIndex ), oColor ) )
                return FALSE;
            m_aoColors[ nIndex ].Set( oColor.GetX(), oColor.GetY(), oColor.GetZ() );
        }
    }
    return TRUE;
}
