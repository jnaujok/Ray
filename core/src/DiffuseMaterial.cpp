#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Vector.h"

#include "DiffuseMaterial.h"
#include "SceneJson.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DIFFUSE_MATERIAL_VERSION    ((DWORD)1)

IMPLEMENT_MATERIAL( CDiffuseMaterial, Diffuse Material, "diffuse" );

CDiffuseMaterial::CDiffuseMaterial( void ) :
m_oColor( 0.5, 0.5, 0.5 )
{
    m_bDiffuseCast = FALSE;
}

CDiffuseMaterial::CDiffuseMaterial( CColor &oColor ) :
m_oColor( oColor )
{
    m_bDiffuseCast = FALSE;
}

CColor CDiffuseMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
        // Un-lit diffuse colors do nothing.
    CColor      oReturn( 0, 0, 0 );

    return oReturn;
}

CColor CDiffuseMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CVector     oToLt( oToLight );
    CColor      oLt( oLight );
    FLT         nIncidentCos;
    CColor      oDiffuse( 0, 0, 0 );
    CVector     oView( oIntersect.GetX() - oViewIncident.GetX(),
                       oIntersect.GetY() - oViewIncident.GetY(),
                       oIntersect.GetZ() - oViewIncident.GetZ() );

    if ( oLt != oDiffuse )
    {
        oToLt.Normalize();

        oNormal.Normalize();
    
            // Are we viewing from the wrong side?
        if ( oNormal.Dot( oView ) > 0.0 )
            oNormal = oNormal * -1.0;

        nIncidentCos = oNormal.Dot( oToLt );
  
        if ( nIncidentCos >= 0.0 )
        {
            oLt = oLt * nIncidentCos;
            oDiffuse = oLt * ColorAt( m_oColor, oIntersect, pIntersect );
        }
    }
    return oDiffuse;
}


#ifdef RAY_GUI
#include "resource.h"
#include "DiffuseMaterialPropDlg.h"

void CDiffuseMaterial::Properties( void )
{
    CDiffuseMaterialPropDlg     oDlg;

    oDlg.m_oColor = m_oColor;
    oDlg.m_bCastDiffuse = m_bDiffuseCast;

    if ( oDlg.DoModal() == IDOK )
    {
        m_oColor = oDlg.m_oColor;
        m_bDiffuseCast = oDlg.m_bCastDiffuse;
    }
}

void CDiffuseMaterial::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddMaterial( this, 14, "Diffuse Material", hUnder );
}

void CDiffuseMaterial::ToList( CListCtrl &oList, int nIndex, FLT nInfluence )
{
    LVITEM      oItem;
    CString     strTemp;

    oItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    oItem.iItem = nIndex;
    oItem.iSubItem = 0;
    oItem.state = 0;
    oItem.stateMask = 0;
    oItem.pszText = "Diffuse";
    oItem.cchTextMax = 0;
    oItem.iImage = 0;
    oItem.lParam = (LPARAM)this;
    oList.InsertItem( &oItem );

    oItem.iSubItem = 1;
    oItem.mask = LVIF_TEXT;
    strTemp.Format( "%f", nInfluence );
    oItem.pszText = (LPSTR)(LPCSTR)strTemp;
    oList.SetItem( &oItem );
}
#endif

void CDiffuseMaterial::ToJson( IASLib::CObjectNode &oNode )
{
    m_oColor.Write( oNode, "color" );
    RayJson::PutBool( oNode, "diffuseCast", m_bDiffuseCast );
}

BOOL CDiffuseMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", "diffuseCast", NULL };

    return RayJson::OnlyFields( oNode, astrFields ) &&
           m_oColor.Read( oNode, "color", FALSE ) &&
           RayJson::GetBool( oNode, "diffuseCast", m_bDiffuseCast, FALSE );
}
