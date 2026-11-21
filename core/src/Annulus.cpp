/*************************************************************************
*   CAnnulus                                                             *
*                                                                        *
*   A flat ring. See Annulus.h.                                          *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Annulus.h"
#include "SceneJson.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CAnnulus, "annulus" );

CAnnulus::CAnnulus( void )
{
    m_nInner = 0.5;
    Prepare();
}

CAnnulus::~CAnnulus( void )
{
}

BOOL CAnnulus::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    if ( ! RayJson::GetNumber( oNode, "radius", m_nRadius, TRUE ) ||
         ! RayJson::GetNumber( oNode, "innerRadius", m_nInner, TRUE ) )
        return FALSE;
    if ( ! ( m_nInner > 0.0 ) )
        return RayJson::Fail( "an annulus's innerRadius must be more than 0" );
    return TRUE;
}

void CAnnulus::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CAnnulus::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
