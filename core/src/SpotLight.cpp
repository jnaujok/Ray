/*************************************************************************
*   CSpotLight                                                           *
*                                                                        *
*   A light that shines in a cone, with a soft edge. See SpotLight.h.    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "SpotLight.h"
#include "Sampling.h"
#include "SceneJson.h"

#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAXIMUM_LIGHT_SAMPLES   256

CSpotLight::CSpotLight( void ) :
m_oDirection( 0.0, -1.0, 0.0 ),
m_nAngle( 30.0 ),
m_nFalloff( 5.0 ),
m_nRadius( 0.0 ),
m_nSamples( 1 )
{
    m_oColor.Set( 1.0, 1.0, 1.0 );
}

CSpotLight::~CSpotLight( void )
{
}

FLT CSpotLight::ConeFactor( const CVector &oToLight ) const
{
    CVector oAxis( m_oDirection );
    CVector oFromLight( oToLight );

    oFromLight = oFromLight * -1.0;

    oAxis.Normalize();
    oFromLight.Normalize();

    FLT nCos = oAxis.Dot( oFromLight );
    FLT nOuter = (FLT)cos( (double)( m_nAngle * PI / 180.0 ) );
    FLT nInner = (FLT)cos( (double)( ( m_nAngle - m_nFalloff ) * PI / 180.0 ) );

    if ( nCos <= nOuter )
        return 0.0;
    if ( nCos >= nInner )
        return 1.0;

        // Smoothstep across the fading edge.
    FLT nT = ( nCos - nOuter ) / ( nInner - nOuter );

    return nT * nT * ( 3.0 - ( 2.0 * nT ) );
}

CColor CSpotLight::GetShade( CVector oToLight )
{
    return GetColor() * ConeFactor( oToLight );
}

BOOL CSpotLight::SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint )
{
        // A point on the disc the light shows oFrom, as a sphere light does.
    CVector oAxis( m_oPosition - oFrom );
    CVector oFirst;
    CVector oSecond;
    FLT     nX;
    FLT     nY;

    oAxis.Normalize();
    if ( oAxis.Magnitude() == 0.0 )
        oAxis.Set( 0.0, 1.0, 0.0 );
    RaySampling::Basis( oAxis, oFirst, oSecond );
    RaySampling::ConcentricDisc( nU, nV, nX, nY );

    CVector oAcross1( oFirst * ( nX * m_nRadius ) );
    CVector oAcross2( oSecond * ( nY * m_nRadius ) );
    CVector oPartway( m_oPosition + oAcross1 );

    oPoint = oPartway + oAcross2;
    return TRUE;
}

#ifdef RAY_GUI
void CSpotLight::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddObject( this, 6, "Spot Light", hUnder );
}
#endif

DYNAMIC_IMPLEMENTS( CSpotLight, "spot" );

void CSpotLight::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "position", m_oPosition );
    RayJson::PutVector( oNode, "direction", m_oDirection );
    RayJson::PutColor( oNode, "color", m_oColor );
    RayJson::PutNumber( oNode, "angle", m_nAngle );
    RayJson::PutNumber( oNode, "falloff", m_nFalloff );
    if ( m_nRadius > 0.0 )
    {
        RayJson::PutNumber( oNode, "radius", m_nRadius );
        oNode.put( "samples", m_nSamples );
    }
}

BOOL CSpotLight::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "position", "direction", "color", "angle", "falloff", "radius", "samples", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetVector( oNode, "position", m_oPosition, TRUE ) ||
         ! RayJson::GetVector( oNode, "direction", m_oDirection, TRUE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, TRUE ) ||
         ! RayJson::GetNumber( oNode, "angle", m_nAngle, FALSE ) ||
         ! RayJson::GetNumber( oNode, "falloff", m_nFalloff, FALSE ) ||
         ! RayJson::GetNumber( oNode, "radius", m_nRadius, FALSE ) ||
         ! RayJson::GetInt( oNode, "samples", m_nSamples, 1, MAXIMUM_LIGHT_SAMPLES, FALSE ) )
        return FALSE;
    if ( m_oDirection.Magnitude() == 0.0 )
        return RayJson::Fail( "direction must not be [0, 0, 0]" );
    if ( ! ( m_nAngle > 0.0 && m_nAngle < 90.0 ) )
        return RayJson::Fail( "angle must be more than 0 and less than 90 degrees" );
    if ( ! ( m_nFalloff >= 0.0 && m_nFalloff <= m_nAngle ) )
        return RayJson::Fail( "falloff must be from 0 to the angle" );
    if ( ! ( m_nRadius >= 0.0 ) )
        return RayJson::Fail( "radius must be 0 or more" );
    return TRUE;
}
