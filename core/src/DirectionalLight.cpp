/*************************************************************************
*   CDirectionalLight                                                    *
*                                                                        *
*   Sunlight, from one direction. See DirectionalLight.h.                *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "DirectionalLight.h"
#include "Sampling.h"
#include "SceneJson.h"

#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAXIMUM_LIGHT_SAMPLES   256

CDirectionalLight::CDirectionalLight( void ) :
m_oDirection( 0.0, -1.0, 0.0 ),
m_nAngle( 0.0 ),
m_nSamples( 1 )
{
    m_oColor.Set( 1.0, 1.0, 1.0 );
}

CDirectionalLight::~CDirectionalLight( void )
{
}

CColor CDirectionalLight::GetShade( CVector oToLight )
{
    (void)oToLight;
    return GetColor();
}

CVector CDirectionalLight::ToLight( CVector &oFrom )
{
    (void)oFrom;

    CVector oBack( m_oDirection );

    oBack.Normalize();
    return oBack * -RAY_FAR_AWAY;
}

BOOL CDirectionalLight::SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint )
{
        // A point on the sun's disc, RAY_FAR_AWAY from oFrom: its radius
        // there is that distance times the tangent of half its angle.
    CVector oAxis( m_oDirection * -1.0 );
    CVector oFirst;
    CVector oSecond;
    FLT     nX;
    FLT     nY;
    FLT     nRadius = RAY_FAR_AWAY * (FLT)tan( (double)( m_nAngle * 0.5 * PI / 180.0 ) );

    oAxis.Normalize();
    RaySampling::Basis( oAxis, oFirst, oSecond );
    RaySampling::ConcentricDisc( nU, nV, nX, nY );

    CVector oOut( oAxis * RAY_FAR_AWAY );
    CVector oCenter( oFrom + oOut );
    CVector oAcross1( oFirst * ( nX * nRadius ) );
    CVector oAcross2( oSecond * ( nY * nRadius ) );
    CVector oPartway( oCenter + oAcross1 );

    oPoint = oPartway + oAcross2;
    return TRUE;
}

#ifdef RAY_GUI
void CDirectionalLight::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddObject( this, 6, "Directional Light", hUnder );
}
#endif

DYNAMIC_IMPLEMENTS( CDirectionalLight, "directional" );

void CDirectionalLight::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "direction", m_oDirection );
    RayJson::PutColor( oNode, "color", m_oColor );
    if ( m_nAngle > 0.0 )
    {
        RayJson::PutNumber( oNode, "angle", m_nAngle );
        oNode.put( "samples", m_nSamples );
    }
}

BOOL CDirectionalLight::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "direction", "color", "angle", "samples", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetVector( oNode, "direction", m_oDirection, TRUE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, TRUE ) ||
         ! RayJson::GetNumber( oNode, "angle", m_nAngle, FALSE ) ||
         ! RayJson::GetInt( oNode, "samples", m_nSamples, 1, MAXIMUM_LIGHT_SAMPLES, FALSE ) )
        return FALSE;
    if ( m_oDirection.Magnitude() == 0.0 )
        return RayJson::Fail( "direction must not be [0, 0, 0]" );
    if ( ! ( m_nAngle >= 0.0 && m_nAngle < 90.0 ) )
        return RayJson::Fail( "angle must be from 0 up to 90 degrees" );
    return TRUE;
}
