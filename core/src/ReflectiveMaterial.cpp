#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "ReflectiveMaterial.h"
#include "Scene.h"
#include "SceneJson.h"
#include "Sampling.h"

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
    m_nRoughness = 0.0;
}

CReflectiveMaterial::CReflectiveMaterial( CColor oColor, FLT nReflect ) :
m_oColor( oColor )
{
    m_nDiffuseReflectivity = nReflect;
    m_bFilterReflection = FALSE;
    m_nRoughness = 0.0;
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

        // A rough mirror reflects about a microfacet normal drawn from
        // around the surface's; one that would send the ray into the
        // surface is passed over for the mirror direction.
    if ( m_nRoughness > 0.0 )
    {
        FLT     nU;
        FLT     nV;
        CVector oFacing( oNormal );

        if ( oFacing.Dot( oView ) > 0.0 )
            oFacing = oFacing * -1.0;
        oViewIncident.SamplePoint( RaySampling::PURPOSE_REFLECT, nU, nV );

        CVector oMicro = RaySampling::Microfacet( oFacing, m_nRoughness, nU, nV );
        CVector oStep = oMicro * ( 2.0 * oMicro.Dot( oView ) );
        CVector oRough = oView - oStep;

        if ( oRough.Dot( oFacing ) > 0.0 )
            oReflection = oRough;
    }

    oReflect.CreateRay( oReflection, oIntersect, oViewIncident.GetInfluence(), oViewIncident.GetPixelX(), oViewIncident.GetPixelY() );
    oReflect.InheritSample( oViewIncident, RaySampling::PURPOSE_REFLECT );

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
    if ( m_nRoughness > 0.0 )
        RayJson::PutNumber( oNode, "roughness", m_nRoughness );
}

BOOL CReflectiveMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", "diffuseReflectivity", "filterReflection",
                                              "roughness", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, FALSE ) ||
         ! RayJson::GetNumber( oNode, "diffuseReflectivity", m_nDiffuseReflectivity, FALSE ) ||
         ! RayJson::GetBool( oNode, "filterReflection", m_bFilterReflection, FALSE ) ||
         ! RayJson::GetNumber( oNode, "roughness", m_nRoughness, FALSE ) )
        return FALSE;
    if ( ( m_nRoughness < 0.0 ) || ( m_nRoughness > 1.0 ) )
        return RayJson::Fail( "roughness must be between 0 and 1" );
    return TRUE;
}
