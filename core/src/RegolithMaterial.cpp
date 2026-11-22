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
m_nWidth( 0.06 ),
m_nOrenNayar( 0.0 ),
m_nOrenNayarWeight( 0.65 )
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

FLT CRegolithMaterial::Factor( FLT nMu0, FLT nMu, FLT nPhase, FLT nCosAzimuth ) const
{
    if ( ! ( m_nOrenNayar > 0.0 ) )
        return Factor( nMu0, nMu, nPhase );
    if ( ! ( nMu0 > 0.0 ) || ( nMu0 + nMu ) <= 0.0 )
        return 0.0;
    if ( nMu < 0.0 )
        nMu = 0.0;

        // Oren-Nayar's qualitative model, scaled so it is 1 seen and lit
        // straight on (as the Lommel-Seeliger term is), then blended.
    double dSigma2 = (double)m_nOrenNayar * (double)m_nOrenNayar;
    double dA = 1.0 - 0.5 * dSigma2 / ( dSigma2 + 0.33 );
    double dB = 0.45 * dSigma2 / ( dSigma2 + 0.09 );
    double dThetaI = acos( (double)( nMu0 > 1.0 ? 1.0 : nMu0 ) );
    double dThetaR = acos( (double)( nMu > 1.0 ? 1.0 : nMu ) );
    double dAlpha = ( dThetaI > dThetaR ) ? dThetaI : dThetaR;
    double dBeta = ( dThetaI > dThetaR ) ? dThetaR : dThetaI;
    double dCos = ( nCosAzimuth > 0.0 ) ? (double)nCosAzimuth : 0.0;
    double dOren = (double)nMu0 * ( dA + dB * dCos * sin( dAlpha ) * tan( dBeta ) ) / dA;
    double dLommel = 2.0 * (double)nMu0 / ( (double)nMu0 + (double)nMu );
    double dWeight = (double)m_nOrenNayarWeight;
    double dSurge = (double)m_nSurge / ( 1.0 + tan( 0.5 * (double)nPhase ) / (double)m_nWidth );

    return (FLT)( ( ( 1.0 - dWeight ) * dLommel + dWeight * dOren ) * ( 1.0 + dSurge ) / ( 1.0 + (double)m_nSurge ) );
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

    FLT nFactor;

    if ( m_nOrenNayar > 0.0 )
    {
            // The azimuth between light and viewer about the normal.
        CVector oNl = oNrm * nMu0;
        CVector oNv = oNrm * nMu;
        CVector oLp = oToLt - oNl;
        CVector oVp = oToEye - oNv;
        FLT     nLen = oLp.Magnitude() * oVp.Magnitude();
        FLT     nCosAz = ( nLen > 1e-12 ) ? oLp.Dot( oVp ) / nLen : 0.0;

        nFactor = Factor( nMu0, nMu, (FLT)acos( (double)nCos ), nCosAz );
    }
    else
        nFactor = Factor( nMu0, nMu, (FLT)acos( (double)nCos ) );

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
    if ( m_nOrenNayar > 0.0 )
    {
        RayJson::PutNumber( oNode, "orenNayar", m_nOrenNayar );
        RayJson::PutNumber( oNode, "orenNayarWeight", m_nOrenNayarWeight );
    }
}

BOOL CRegolithMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", "surge", "width", "orenNayar", "orenNayarWeight", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! m_oColor.Read( oNode, "color", FALSE ) ||
         ! RayJson::GetNumber( oNode, "surge", m_nSurge, FALSE ) ||
         ! RayJson::GetNumber( oNode, "width", m_nWidth, FALSE ) ||
         ! RayJson::GetNumber( oNode, "orenNayar", m_nOrenNayar, FALSE ) ||
         ! RayJson::GetNumber( oNode, "orenNayarWeight", m_nOrenNayarWeight, FALSE ) )
        return FALSE;
    if ( ! ( m_nOrenNayar >= 0.0 ) || ! ( m_nOrenNayar <= 1.5 ) )
        return RayJson::Fail( "orenNayar (the Oren-Nayar roughness, in radians) must be from 0 to 1.5" );
    if ( ! ( m_nOrenNayarWeight >= 0.0 ) || ! ( m_nOrenNayarWeight <= 1.0 ) )
        return RayJson::Fail( "orenNayarWeight must be from 0 to 1" );
    if ( ! ( m_nSurge >= 0.0 ) || ! isfinite( (double)m_nSurge ) )
        return RayJson::Fail( "surge (the opposition surge's strength) can't be negative" );
    if ( ! ( m_nWidth > 0.0 ) || ! isfinite( (double)m_nWidth ) )
        return RayJson::Fail( "width (the opposition surge's angular width) must be positive" );
    return TRUE;
}
