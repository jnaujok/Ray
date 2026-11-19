#ifdef _WINDOWS
    #include "StdAfx.h"
    #include "resource.h"
    #include "SpecMatDlg.h"
#else
    #include "Globals.h"
#endif

#include "Vector.h"
#include "SpecularMaterial.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define SPECULAR_MATERIAL_VERSION    ((DWORD)1)

IMPLEMENT_MATERIAL( CSpecularMaterial, Specular Material );

CSpecularMaterial::CSpecularMaterial( void ) :
m_oColor( 0.5, 0.5, 0.5 )
{
    m_nPower = 1.0;
    m_nIOR = 1.0;
    m_bMaterialHighlight = FALSE;
}

CSpecularMaterial::CSpecularMaterial( CColor oColor, FLT nPower ) :
m_oColor( oColor )
{
    m_nPower = nPower;
    m_nIOR = 1.0;
    m_bMaterialHighlight = FALSE;
}

CStream &operator <<( CStream &oAr, CSpecularMaterial &oMaterial )
{
    oAr << SPECULAR_MATERIAL_VERSION;
    oAr << oMaterial.m_oColor;
    oAr << oMaterial.m_nPower;
    oAr << oMaterial.m_nIOR;
    oAr << oMaterial.m_bMaterialHighlight;
    return oAr;
}

CStream &operator >>( CStream &oAr, CSpecularMaterial &oMaterial )
{
    DWORD   dwVersion;

    oAr >> dwVersion;

    switch ( dwVersion )
    {
        case SPECULAR_MATERIAL_VERSION:
            oAr >> oMaterial.m_oColor;
            oAr >> oMaterial.m_nPower;
            oAr >> oMaterial.m_nIOR;
            oAr >> oMaterial.m_bMaterialHighlight;
            break;

        default:
            break;
    }
    return oAr;
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
    FLT         nMSquared = m_nPower * m_nPower;
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
    
        nNDotL = oNormal.Dot( oToLt );

        oView.Normalize();

        oH = oToLt + oView;
        oH = oH * 0.5;

        oH.Normalize();

        nNDotV = oNormal.Dot( oView );

        nNDotH = oNormal.Dot( oH );

        nVDotH = oView.Dot( oH );

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

        c = nVDotH;
        g = SQRT( ( m_nIOR * m_nIOR ) + ( c * c ) - 1);

        gMinusc = g - c;
        gPlusc = g + c;

        x = ( gMinusc * gMinusc ) / ( gPlusc * gPlusc );
        y = (c * gPlusc) - 1.0;
        y *= y;
        z = (c * gMinusc) + 1.0;
        z *= z;


        nF = 0.50 * ( x * ( 1 + ( y / z ) ) );

        nRs = ( nF * nD * nG ) / nDenom;

        oLt = oLt * ( nNDotL * nRs );

        oSpecular = oLt * m_oColor;
    }
    return oSpecular;
}


#ifdef _WINDOWS
void CSpecularMaterial::Properties( void )
{
    CSpecMatDlg oDlg;

    oDlg.m_oColor = m_oColor;
    oDlg.m_nIORValue = m_nIOR;
    oDlg.m_nPower = m_nPower;
    oDlg.m_bMaterialHighlights = m_bMaterialHighlight;

    if ( oDlg.DoModal() == IDOK )
    {
        m_oColor = oDlg.m_oColor;
        m_nIOR = oDlg.m_nIORValue;
        m_nPower = oDlg.m_nPower;
        m_bMaterialHighlight = oDlg.m_bMaterialHighlights;
    }
}

void  CSpecularMaterial::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddMaterial( this, 16, "Specular Material", hUnder );
}

void CSpecularMaterial::ToList( CListCtrl &oList, int nIndex, FLT nInfluence )
{
    LVITEM      oItem;
    CString     strTemp;

    oItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    oItem.iItem = nIndex;
    oItem.iSubItem = 0;
    oItem.state = 0;
    oItem.stateMask = 0;
    oItem.pszText = "Specular";
    oItem.cchTextMax = 0;
    oItem.iImage = 1;
    oItem.lParam = (LPARAM)this;
    oList.InsertItem( &oItem );

    oItem.iSubItem = 1;
    oItem.mask = LVIF_TEXT;
    strTemp.Format( "%f", nInfluence );
    oItem.pszText = (LPSTR)(LPCSTR)strTemp;
    oList.SetItem( &oItem );
}

#endif