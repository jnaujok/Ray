#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "ReflectiveMaterial.h"
#include "Scene.h"
#include "SceneJson.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define REFLECTIVE_MATERIAL_VERSION    ((DWORD)1)

IMPLEMENT_MATERIAL( CReflectiveMaterial, Reflective Material, "reflective" );

CReflectiveMaterial::CReflectiveMaterial( void ) :
m_oColor( 0.5, 0.5, 0.5 )
{
    m_nDiffuseReflectivity = 0.00;
    m_bFilterReflection = FALSE;
}

CReflectiveMaterial::CReflectiveMaterial( CColor oColor, FLT nReflect ) :
m_oColor( oColor )
{
    m_nDiffuseReflectivity = nReflect;
    m_bFilterReflection = FALSE;
}

CColor CReflectiveMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CColor      oReturn(0,0,0);

    return oReturn;
}

CColor CReflectiveMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CColor      oReflective;
    CVector     oView( oIntersect.GetX() - oViewIncident.GetX(),
                       oIntersect.GetY() - oViewIncident.GetY(),
                       oIntersect.GetZ() - oViewIncident.GetZ() );
    CRay        oReflect;
    CVector     oReflection;
    CVector     o2N;

    oNormal.Normalize();

    oView.Normalize();

        // The mirror direction, R = V - 2 ( N.V ) N, whichever way the
        // normal faces.
    o2N = oNormal * ( 2.0 * oNormal.Dot( oView ) );
    oReflection = oView - o2N;

    oReflect.CreateRay( oReflection, oIntersect, oViewIncident.GetInfluence(), oViewIncident.GetPixelX(), oViewIncident.GetPixelY() );

        // The above function is tailored for Shadow rays, so we need to stretch
        // out the valid intersection range.

    oReflect.SetT( 1e100 );
    oReflect.SetDepth( oViewIncident.GetDepth() );

    oScene.TraceRay( oReflect, oReflective, pIntersect );

    if ( m_bFilterReflection )
    {
        oReflective = oReflective * ColorAt( m_oColor, oIntersect, pIntersect );
    }

    return oReflective;
}


#ifdef RAY_GUI
#include "resource.h"
#include "ReflectiveMatDlg.h"

void CReflectiveMaterial::Properties( void )
{
    CReflectiveMatDlg       oDlg;

    oDlg.m_oColor = m_oColor;
    oDlg.m_bFilterReflection = m_bFilterReflection;
    oDlg.m_nDiffuseReflection = m_nDiffuseReflectivity;

    if ( oDlg.DoModal() == IDOK )
    {
        m_oColor = oDlg.m_oColor;
        m_bFilterReflection = oDlg.m_bFilterReflection;
        m_nDiffuseReflectivity = oDlg.m_nDiffuseReflection;
    }
}

void  CReflectiveMaterial::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddMaterial( this, 18, "Reflective Material", hUnder );
}

void CReflectiveMaterial::ToList( CListCtrl &oList, int nIndex, FLT nInfluence )
{
    LVITEM      oItem;
    CString     strTemp;

    oItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    oItem.iItem = nIndex;
    oItem.iSubItem = 0;
    oItem.state = 0;
    oItem.stateMask = 0;
    oItem.pszText = "Reflective";
    oItem.cchTextMax = 0;
    oItem.iImage = 2;
    oItem.lParam = (LPARAM)this;
    oList.InsertItem( &oItem );

    oItem.iSubItem = 1;
    oItem.mask = LVIF_TEXT;
    strTemp.Format( "%f", nInfluence );
    oItem.pszText = (LPSTR)(LPCSTR)strTemp;
    oList.SetItem( &oItem );
}

#endif

void CReflectiveMaterial::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutColor( oNode, "color", m_oColor );
    RayJson::PutNumber( oNode, "diffuseReflectivity", m_nDiffuseReflectivity );
    RayJson::PutBool( oNode, "filterReflection", m_bFilterReflection );
}

BOOL CReflectiveMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", "diffuseReflectivity", "filterReflection", NULL };

    return RayJson::OnlyFields( oNode, astrFields ) &&
           RayJson::GetColor( oNode, "color", m_oColor, FALSE ) &&
           RayJson::GetNumber( oNode, "diffuseReflectivity", m_nDiffuseReflectivity, FALSE ) &&
           RayJson::GetBool( oNode, "filterReflection", m_bFilterReflection, FALSE );
}
