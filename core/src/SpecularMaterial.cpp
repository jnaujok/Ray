    #include "Globals.h"

#include "Vector.h"
#include "SpecularMaterial.h"
#include "SceneJson.h"



#define SPECULAR_MATERIAL_VERSION    ((DWORD)1)

IMPLEMENT_MATERIAL( CSpecularMaterial, Specular Material, "specular" );

CSpecularMaterial::CSpecularMaterial( void ) :
m_oColor( 0.5, 0.5, 0.5 )
{
    m_nPower.SetRange( 0.001, 100.0 );
    m_nIOR.SetRange( 0.001, 50.0 );
    m_nPower = 1.0;
    m_nIOR = 1.5;           // an index of 1 gives no Fresnel reflection at all
    m_bMaterialHighlight = FALSE;
}

CSpecularMaterial::CSpecularMaterial( CColor oColor, FLT nPower ) :
m_oColor( oColor )
{
    m_nPower.SetRange( 0.001, 100.0 );
    m_nIOR.SetRange( 0.001, 50.0 );
    m_nPower = nPower;
    m_nIOR = 1.5;
    m_bMaterialHighlight = FALSE;
}

CColor CSpecularMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CColor      oReturn( 0, 0, 0 );

    return oReturn;
}

CColor CSpecularMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CVector     oToLt( oToLight );
    CVector     oView( oViewIncident.GetX() - oIntersect.GetX(),
                       oViewIncident.GetY() - oIntersect.GetY(),
                       oViewIncident.GetZ() - oIntersect.GetZ() );
    CColor      oLt( oLight );
    CColor      oSpecular( 0, 0, 0 );
    FLT         nNDotV;
    FLT         nNDotL;
    FLT         nDenom;
    CVector     oH;
    FLT         nNDotH;
    FLT         nVDotH;
    FLT         nF;
    FLT         nG;
    FLT         nGMask;
    FLT         nGShadow;
    FLT         nD;
    FLT         nCosAlpha;
    FLT         nSinAlpha;
    FLT         nTanAlpha;
    CMapPoint   oAt( oIntersect, pIntersect );
    FLT         nPower = m_nPower.At( oAt );
    FLT         nMSquared = nPower * nPower;
    FLT         c;
    FLT         g;
    FLT         gMinusc;
    FLT         gPlusc;
    FLT         x, y, z;
    FLT         nRs;

        // Is the light black (ie are we outside of the light cone.)?
    if ( oLt != oSpecular )
    {
        oToLt.Normalize();

        oNormal.Normalize();

        oView.Normalize();

            // Shade the side the viewer sees, as the diffuse material does.
        if ( oNormal.Dot( oView ) < 0.0 )
            oNormal = oNormal * -1.0;

        nNDotL = oNormal.Dot( oToLt );
        nNDotV = oNormal.Dot( oView );

            // A light behind the surface, or a surface seen edge on, gives no
            // highlight (and would divide by zero below).
        if ( ( nNDotL <= 0.0 ) || ( nNDotV <= 0.0 ) )
            return oSpecular;

        oH = oToLt + oView;
        oH = oH * 0.5;

        oH.Normalize();

        nNDotH = oNormal.Dot( oH );

        nVDotH = oView.Dot( oH );

        if ( ( nNDotH <= 0.0 ) || ( nVDotH <= 0.0 ) )
            return oSpecular;

        nDenom = PI * nNDotL * nNDotV;

        nCosAlpha = nNDotH;
        nSinAlpha = SQRT( 1 - ( nCosAlpha * nCosAlpha ) );

        if ( nCosAlpha != 0.0 )
            nTanAlpha = nSinAlpha / nCosAlpha;
        else
            nTanAlpha = 1000000.0;

        nD = ( 1.0 / ( nMSquared * POW( nCosAlpha, 4.0 ) ) ) * ( EXP( - ( ( nTanAlpha * nTanAlpha ) / nMSquared ) ) );

        nGMask = 2.0 * ( ( nNDotH * nNDotV ) / nVDotH );
        nGShadow = 2.0 * ( ( nNDotH * nNDotL ) / nVDotH );

        if ( nGMask < 0.0 )
            nGMask = 0.0;

        if ( nGShadow < 0.0 )
            nGShadow = 0.0;

        if ( ( nGMask < nGShadow ) && ( nGMask < 1.0 ) )
            nG = nGMask;
        else if ( ( nGShadow < nGMask ) && ( nGShadow < 1.0 ) )
            nG = nGShadow;
        else if ( nGShadow < 1.0 )
            nG = nGShadow;
        else
            nG = 1.0;

            // Fresnel, for unpolarized light: with c = V.H and
            // g = sqrt( n^2 + c^2 - 1 ),
            //   F = ( g - c )^2 / ( 2 ( g + c )^2 ) * ( 1 + ( ( c ( g + c ) - 1 ) / ( c ( g - c ) + 1 ) )^2 ).
            // An index below 1 can leave nothing under the root: all of it
            // is reflected.
        c = nVDotH;
        FLT nIOR = m_nIOR.At( oAt );

        g = ( nIOR * nIOR ) + ( c * c ) - 1.0;

        if ( g <= 0.0 )
            nF = 1.0;
        else
        {
            g = SQRT( g );
            gMinusc = g - c;
            gPlusc = g + c;

            x = ( gMinusc * gMinusc ) / ( gPlusc * gPlusc );
            y = (c * gPlusc) - 1.0;
            y *= y;
            z = (c * gMinusc) + 1.0;
            z *= z;

            nF = 0.50 * ( x * ( 1 + ( y / z ) ) );
        }

        nRs = ( nF * nD * nG ) / nDenom;

        oLt = oLt * ( nNDotL * nRs );

        oSpecular = oLt * ColorAt( m_oColor, oIntersect, pIntersect );
    }
    return oSpecular;
}



void CSpecularMaterial::ToJson( IASLib::CObjectNode &oNode )
{
    m_oColor.Write( oNode, "color" );
    m_nPower.Write( oNode, "power" );
    m_nIOR.Write( oNode, "indexOfRefraction" );
    RayJson::PutBool( oNode, "materialHighlight", m_bMaterialHighlight );
}

BOOL CSpecularMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", "power", "indexOfRefraction", "materialHighlight", NULL };

    return RayJson::OnlyFields( oNode, astrFields ) &&
           m_oColor.Read( oNode, "color", FALSE ) &&
           m_nPower.Read( oNode, "power", FALSE ) &&
           m_nIOR.Read( oNode, "indexOfRefraction", FALSE ) &&
           RayJson::GetBool( oNode, "materialHighlight", m_bMaterialHighlight, FALSE );
}
