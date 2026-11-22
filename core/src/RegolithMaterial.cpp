    #include "Globals.h"

#include "Vector.h"
#include "Rays.h"

#include "RegolithMaterial.h"
#include "SceneJson.h"

#include <math.h>

IMPLEMENT_MATERIAL( CRegolithMaterial, Regolith Material, "regolith" );

CRegolithMaterial::CRegolithMaterial( void ) :
m_oColor( 0.12, 0.12, 0.12 ),
m_nSurge( 1.0 ),
m_nWidth( 0.06 )
{
}

FLT CRegolithMaterial::Factor( FLT nMu0, FLT nMu, FLT nPhase ) const
{
    if ( ! ( nMu0 > 0.0 ) || ( nMu0 + nMu ) <= 0.0 )
        return 0.0;
    if ( nMu < 0.0 )
        nMu = 0.0;

    double dSurge = (double)m_nSurge / ( 1.0 + tan( 0.5 * (double)nPhase ) / (double)m_nWidth );

    return (FLT)( ( 2.0 * nMu0 / ( nMu0 + nMu ) ) * ( 1.0 + dSurge ) / ( 1.0 + (double)m_nSurge ) );
}

CColor CRegolithMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    return CColor( 0, 0, 0 );
}

CColor CRegolithMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CColor  oBlack( 0, 0, 0 );
    CColor  oLt( oLight );

    if ( oLt == oBlack )
        return oBlack;

    CVector oToLt( oToLight );
    CVector oToEye( oViewIncident.GetX() - oIntersect.GetX(),
                    oViewIncident.GetY() - oIntersect.GetY(),
                    oViewIncident.GetZ() - oIntersect.GetZ() );
    CVector oNrm( oNormal );

    oToLt.Normalize();
    oToEye.Normalize();
    oNrm.Normalize();
    if ( oNrm.Dot( oToEye ) < 0.0 )
        oNrm = oNrm * -1.0;

    FLT nMu0 = oNrm.Dot( oToLt );
    FLT nMu = oNrm.Dot( oToEye );
    FLT nCos = oToLt.Dot( oToEye );

    if ( nCos > 1.0 )
        nCos = 1.0;
    if ( nCos < -1.0 )
        nCos = -1.0;

    FLT nFactor = Factor( nMu0, nMu, (FLT)acos( (double)nCos ) );

    if ( ! ( nFactor > 0.0 ) )
        return oBlack;
    oLt = oLt * nFactor;
    return oLt * ColorAt( m_oColor, oIntersect, pIntersect );
}


void CRegolithMaterial::ToJson( IASLib::CObjectNode &oNode )
{
    m_oColor.Write( oNode, "color" );
    RayJson::PutNumber( oNode, "surge", m_nSurge );
    RayJson::PutNumber( oNode, "width", m_nWidth );
}

BOOL CRegolithMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", "surge", "width", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! m_oColor.Read( oNode, "color", FALSE ) ||
         ! RayJson::GetNumber( oNode, "surge", m_nSurge, FALSE ) ||
         ! RayJson::GetNumber( oNode, "width", m_nWidth, FALSE ) )
        return FALSE;
    if ( ! ( m_nSurge >= 0.0 ) || ! isfinite( (double)m_nSurge ) )
        return RayJson::Fail( "surge (the opposition surge's strength) can't be negative" );
    if ( ! ( m_nWidth > 0.0 ) || ! isfinite( (double)m_nWidth ) )
        return RayJson::Fail( "width (the opposition surge's angular width) must be positive" );
    return TRUE;
}
