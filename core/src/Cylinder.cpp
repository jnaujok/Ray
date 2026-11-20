/*************************************************************************
*   CCylinder                                                            *
*                                                                        *
*   A cylinder: a cone with equal radii. See Cylinder.h.                 *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Cylinder.h"
#include "SceneJson.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCylinder::CCylinder( void )
{
    m_nTopRadius = m_nBaseRadius;
    Prepare();
}

CCylinder::~CCylinder( void )
{
}

#ifdef RAY_GUI
void CCylinder::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hCylinder;

    hCylinder = pTree->AddObject( this, 8, "Cylinder", hUnder );
    CObjects::MatToTree( pTree, hCylinder );
}
#endif

DYNAMIC_IMPLEMENTS( CCylinder, "cylinder" );

void CCylinder::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "base", m_oBase );
    RayJson::PutVector( oNode, "top", m_oTop );
    RayJson::PutNumber( oNode, "radius", m_nBaseRadius );
    RayJson::PutBool( oNode, "capped", m_bCapped );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
}

BOOL CCylinder::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "base", "top", "radius", "capped", "color", "materials", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetNumber( oNode, "radius", m_nBaseRadius, TRUE ) )
        return FALSE;
    if ( ! ( m_nBaseRadius > 0.0 ) )
        return RayJson::Fail( "the radius must be more than 0" );
    m_nTopRadius = m_nBaseRadius;
    return ReadEnds( oNode ) && Prepare();
}
