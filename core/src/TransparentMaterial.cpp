#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "TransparentMaterial.h"
#include "Scene.h"
#include "SceneJson.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TRANSPARENT_MATERIAL_VERSION    ((DWORD)1)

IMPLEMENT_MATERIAL( CTransparentMaterial, Transparent Material, "transparent" );

CTransparentMaterial::CTransparentMaterial( void ) :
m_oColor( 0.5, 0.5, 0.5 )
{
    m_nIndexOfRefraction = 1.0;
}

CTransparentMaterial::CTransparentMaterial( CColor oColor, FLT nRefract ) :
m_oColor( oColor )
{
    m_nIndexOfRefraction = nRefract;
}

CColor CTransparentMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CColor      oReturn( 0, 0, 0 );

    return oReturn;
}

BOOL CTransparentMaterial::Refract( CVector oView, CVector oNormal, FLT nIndex, CVector &oTransmit )
{
    FLT         nIOR;
    FLT         nCosTheta1;
    FLT         nCosTheta2;
    CVector     oStep;

    oNormal.Normalize();
    oView.Normalize();

        // Snell's law in vector form. With the normal turned against the
        // ray (so cos1 = -N.V is positive) and eta = n1 / n2:
        //   cos2^2 = 1 - eta^2 ( 1 - cos1^2 )
        //   T = eta V + ( eta cos1 - cos2 ) N
        // Entering the glass (the ray against the outward normal), eta is
        // 1 / n; leaving it, n.
    nCosTheta1 = -oNormal.Dot( oView );

    if ( nCosTheta1 >= 0.0 )
        nIOR = 1.0 / nIndex;
    else
    {
        nIOR = nIndex;
        oNormal = oNormal * -1.0;
        nCosTheta1 = -nCosTheta1;
    }

    nCosTheta2 = 1.0 - ( nIOR * nIOR * ( 1.0 - ( nCosTheta1 * nCosTheta1 ) ) );

    if ( nCosTheta2 < 0.0 )
    {
            // Total internal reflection: past the critical angle, inside the
            // denser glass, nothing gets out and the ray reflects.
        oStep = oNormal * ( 2.0 * nCosTheta1 );
        oTransmit = oView + oStep;
        return FALSE;
    }

    CVector oViewStep = oView * nIOR;

    nCosTheta2 = SQRT( nCosTheta2 );
    oStep = oNormal * ( ( nIOR * nCosTheta1 ) - nCosTheta2 );
    oTransmit = oViewStep + oStep;
    return TRUE;
}

CColor CTransparentMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CColor      oTransparent;
    CVector     oView( oIntersect.GetX() - oViewIncident.GetX(),
                       oIntersect.GetY() - oViewIncident.GetY(),
                       oIntersect.GetZ() - oViewIncident.GetZ() );
    CRay        oTransmitted;
    CVector     oTransmit;
    CVector     oTemp;
    CVector     oTransStep;

    oNormal.Normalize();

    oView.Normalize();

    Refract( oView, oNormal, m_nIndexOfRefraction, oTransmit );

    oTransStep = oTransmit * 0.01;

    oTemp = oIntersect + oTransStep;

    oTransmitted.CreateRay( oTransmit, oTemp, oViewIncident.GetInfluence(), oViewIncident.GetPixelX(), oViewIncident.GetPixelY() );

        // The above function is tailored for Shadow rays, so we need to stretch
        // out the valid intersection range.

    oTransmitted.SetT( 1e100 );
    oTransmitted.SetDepth( oViewIncident.GetDepth() );

    oScene.TraceRay( oTransmitted, oTransparent, NULL );

    oTransparent = oTransparent * ColorAt( m_oColor, oIntersect, pIntersect );

    return oTransparent;
}


#ifdef RAY_GUI
#include "resource.h"
#include "TransparentMatDlg.h"

void CTransparentMaterial::Properties( void )
{
    CTransparentMatDlg       oDlg;

    oDlg.m_oColor = m_oColor;
    oDlg.m_nIndexOfRefraction = m_nIndexOfRefraction;

    if ( oDlg.DoModal() == IDOK )
    {
        m_oColor = oDlg.m_oColor;
        m_nIndexOfRefraction = oDlg.m_nIndexOfRefraction;
    }
}

void  CTransparentMaterial::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddMaterial( this, 20, "Transparent Material", hUnder );
}

void CTransparentMaterial::ToList( CListCtrl &oList, int nIndex, FLT nInfluence )
{
    LVITEM      oItem;
    CString     strTemp;

    oItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    oItem.iItem = nIndex;
    oItem.iSubItem = 0;
    oItem.state = 0;
    oItem.stateMask = 0;
    oItem.pszText = "Transparent";
    oItem.cchTextMax = 0;
    oItem.iImage = 3;
    oItem.lParam = (LPARAM)this;
    oList.InsertItem( &oItem );

    oItem.iSubItem = 1;
    oItem.mask = LVIF_TEXT;
    strTemp.Format( "%f", nInfluence );
    oItem.pszText = (LPSTR)(LPCSTR)strTemp;
    oList.SetItem( &oItem );
}

#endif

void CTransparentMaterial::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutColor( oNode, "color", m_oColor );
    RayJson::PutNumber( oNode, "indexOfRefraction", m_nIndexOfRefraction );
}

BOOL CTransparentMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", "indexOfRefraction", NULL };

    return RayJson::OnlyFields( oNode, astrFields ) &&
           RayJson::GetColor( oNode, "color", m_oColor, FALSE ) &&
           RayJson::GetNumber( oNode, "indexOfRefraction", m_nIndexOfRefraction, FALSE );
}
