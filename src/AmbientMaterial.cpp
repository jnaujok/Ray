#ifdef _WINDOWS
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Vector.h"

#include "AmbientMaterial.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define AMBIENT_MATERIAL_VERSION    ((DWORD)1)

IMPLEMENT_MATERIAL( CAmbientMaterial, Ambient Material );

CAmbientMaterial::CAmbientMaterial( void ) :
m_oColor( .3, .3, .3 )
{
}

CAmbientMaterial::CAmbientMaterial( CColor &oColor ) :
m_oColor( oColor )
{
}

CStream &operator <<( CStream &oAr, CAmbientMaterial &oMaterial )
{
    oAr << AMBIENT_MATERIAL_VERSION;
    oAr << oMaterial.m_oColor;

    return oAr;
}

CStream &operator >>( CStream &oAr, CAmbientMaterial &oMaterial )
{
    DWORD   dwVersion;

    oAr >> dwVersion;

    switch ( dwVersion )
    {
        case AMBIENT_MATERIAL_VERSION:
            oAr >> oMaterial.m_oColor;
            break;

        default:
            break;
    }
    return oAr;
}

CColor CAmbientMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
        // Simply return the ambient color and let the influence adjust the light. 
        // That's what ambient means...
    return m_oColor;
}

CColor CAmbientMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
        // Ambient color does not depend on light sources
    CColor      oAmbient( 0, 0, 0 );

    return oAmbient;
}


#ifdef _WINDOWS
#include "resource.h"
#include "SetColorDlg.h"

void CAmbientMaterial::Properties( void )
{
    CSetColorDlg    oDlg( &m_oColor );

    oDlg.DoModal();
}

void CAmbientMaterial::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddMaterial( this, 22, "Ambient Material", hUnder );
}

void CAmbientMaterial::ToList( CListCtrl &oList, int nIndex, FLT nInfluence )
{
    LVITEM      oItem;
    CString     strTemp;

    oItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    oItem.iItem = nIndex;
    oItem.iSubItem = 0;
    oItem.state = 0;
    oItem.stateMask = 0;
    oItem.pszText = "Ambient";
    oItem.cchTextMax = 0;
    oItem.iImage = 4;
    oItem.lParam = (LPARAM)this;
    oList.InsertItem( &oItem );

    oItem.iSubItem = 1;
    oItem.mask = LVIF_TEXT;
    strTemp.Format( "%f", nInfluence );
    oItem.pszText = (LPSTR)(LPCSTR)strTemp;
    oList.SetItem( &oItem );
}
#endif
