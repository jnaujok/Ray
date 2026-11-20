/*************************************************************************
*   CRectangleLight                                                      *
*                                                                        *
*   A glowing panel: an area light that casts soft shadows. See          *
*   RectangleLight.h.                                                    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "RectangleLight.h"
#include "SceneJson.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAXIMUM_LIGHT_SAMPLES   256

CRectangleLight::CRectangleLight( void ) :
m_oEdge1( 1.0, 0.0, 0.0 ),
m_oEdge2( 0.0, 0.0, 1.0 ),
m_nSamples( 1 )
{
    m_oColor.Set( 1.0, 1.0, 1.0 );
}

CRectangleLight::~CRectangleLight( void )
{
}

CColor CRectangleLight::GetShade( CVector oVector )
{
    (void)oVector;
    return GetColor();
}

BOOL CRectangleLight::SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint )
{
    CVector oFacing( m_oEdge1.Cross( m_oEdge2 ) );
    CVector oAway( oFrom - m_oPosition );

        // Only the side the panel faces is lit.
    if ( ! ( oFacing.Dot( oAway ) > 0.0 ) )
        return FALSE;

    CVector oAlong1( m_oEdge1 * ( nU - 0.5 ) );
    CVector oAlong2( m_oEdge2 * ( nV - 0.5 ) );
    CVector oPartway( m_oPosition + oAlong1 );

    oPoint = oPartway + oAlong2;
    return TRUE;
}

#ifdef RAY_GUI
void CRectangleLight::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddObject( this, 6, "Rectangle Light", hUnder );
}
#endif

DYNAMIC_IMPLEMENTS( CRectangleLight, "rectangleLight" );

void CRectangleLight::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "position", m_oPosition );
    RayJson::PutColor( oNode, "color", m_oColor );
    RayJson::PutVector( oNode, "edge1", m_oEdge1 );
    RayJson::PutVector( oNode, "edge2", m_oEdge2 );
    oNode.put( "samples", m_nSamples );
}

BOOL CRectangleLight::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "position", "color", "edge1", "edge2", "samples", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetVector( oNode, "position", m_oPosition, TRUE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, TRUE ) ||
         ! RayJson::GetVector( oNode, "edge1", m_oEdge1, TRUE ) ||
         ! RayJson::GetVector( oNode, "edge2", m_oEdge2, TRUE ) ||
         ! RayJson::GetInt( oNode, "samples", m_nSamples, 1, MAXIMUM_LIGHT_SAMPLES, FALSE ) )
        return FALSE;

    CVector oFacing( m_oEdge1.Cross( m_oEdge2 ) );

    if ( ! ( oFacing.Magnitude() > 0.0 ) )
        return RayJson::Fail( "edge1 and edge2 must be sides of a rectangle: not zero, and not along the same line" );
    return TRUE;
}
