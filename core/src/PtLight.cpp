/*************************************************************************
*   CPointLight                                                          *
*                                                                        *
*       This class defines the basic point light source which is ever so *
* popular in graphics. All light from a point source is produced from a  *
* single non-dimensional point in space. Light from the point source is  *
* evenly distributed at all angles and does not fall off with distance.  *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
    #include "resource.h"
    #include "PointLightPropertiesDlg.h"
#else
    #include "Globals.h"
#endif

#include "PtLight.h"
#include "SceneJson.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define PTLIGHT_VERSION     ((DWORD)1)

CPointLight::CPointLight( void )
{
}

CPointLight::CPointLight( FLT nRed, FLT nGreen, FLT nBlue, FLT nX, FLT nY, FLT nZ ) :
CLight( nRed, nGreen, nBlue, nX, nY, nZ )
{
}

CPointLight::CPointLight( CVector oColor, CVector oPos ) :
CLight( oColor.GetX(), oColor.GetY(), oColor.GetZ(), oPos.GetX(), oPos.GetY(), oPos.GetZ() )
{
}


CPointLight::~CPointLight( void )
{
}

CColor CPointLight::GetShade( CVector oVector )
{
    CColor  oReturn( GetColor() );

    // Note that a point light is omni directional, so there is no difference for angle of
    // entry. This is useful only for spot lights and directional lights.

    return oReturn;
}

#ifdef RAY_GUI
void CPointLight::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddObject( this, 6, "Point Light", hUnder );
}

void CPointLight::SetProperties( void )
{
    CPointLightPropertiesDlg     oProp( &m_oPosition, &m_oColor );

    oProp.DoModal();
}
#endif

DYNAMIC_IMPLEMENTS( CPointLight, "point" );

void CPointLight::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "position", m_oPosition );
    RayJson::PutColor( oNode, "color", m_oColor );
    OptionsToJson( oNode, FALSE );
}

BOOL CPointLight::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "position", "color", NULL };

    return OnlyLightFields( oNode, astrFields, TRUE, FALSE ) &&
           RayJson::GetVector( oNode, "position", m_oPosition, TRUE ) &&
           RayJson::GetColor( oNode, "color", m_oColor, TRUE ) &&
           OptionsFromJson( oNode, TRUE, FALSE );
}
