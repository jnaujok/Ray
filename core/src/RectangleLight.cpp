/*************************************************************************
*   CRectangleLight                                                      *
*                                                                        *
*   A glowing panel: an area light that casts soft shadows. See          *
*   RectangleLight.h.                                                    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "RectangleLight.h"
#include "SceneJson.h"

#include <math.h>
#include <algorithm>


#define MAXIMUM_LIGHT_SAMPLES   256

CRectangleLight::CRectangleLight( void ) :
m_oEdge1( 1.0, 0.0, 0.0 ),
m_oEdge2( 0.0, 0.0, 1.0 ),
m_nSamples( 1 ),
m_nAngle( 0.0 ),
m_nFalloff( 0.0 )
{
    m_oColor.Set( 1.0, 1.0, 1.0 );
}

CRectangleLight::~CRectangleLight( void )
{
}

CColor CRectangleLight::GetShade( CVector oVector )
{
    if ( ! ( m_nAngle > 0.0 ) )
        return GetColor();

        // In a beam: by the angle between its facing and the way the light
        // leaves it.
    CVector oFacing( m_oEdge1.Cross( m_oEdge2 ) );
    CVector oFromLight( oVector );

    oFromLight = oFromLight * -1.0;
    oFacing.Normalize();
    oFromLight.Normalize();
    return GetColor() * ProfileFactor( oFacing.Dot( oFromLight ), m_nAngle, m_nFalloff );
}

void CRectangleLight::FilterAt( const CVector &oPoint, const CVector &oToLight, STexturePoint &oAt ) const
{
    CVector oEdge1( m_oEdge1 );
    CVector oEdge2( m_oEdge2 );
    CVector oFacing( oEdge1.Cross( oEdge2 ) );
    CVector oFrom( oPoint );
    CVector oCenter( m_oPosition );
    CVector oRelative( oFrom - oCenter );

    (void)oToLight;
    oFacing.Normalize();

    FLT     nAbove = oFacing.Dot( oRelative );
    CVector oUp( oFacing * nAbove );
    CVector oOnPlane( oRelative - oUp );
    FLT     nLength1 = oEdge1.Dot( oEdge1 );
    FLT     nLength2 = oEdge2.Dot( oEdge2 );

    oAt.nU = ( oEdge1.Dot( oOnPlane ) / nLength1 ) + 0.5;
    oAt.nV = ( oEdge2.Dot( oOnPlane ) / nLength2 ) + 0.5;
    oAt.oPoint = oOnPlane * ( 1.0 / m_nFilterSize );
    oAt.bHasUV = TRUE;
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


DYNAMIC_IMPLEMENTS( CRectangleLight, "rectangleLight" );

void CRectangleLight::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "position", m_oPosition );
    RayJson::PutColor( oNode, "color", m_oColor );
    RayJson::PutVector( oNode, "edge1", m_oEdge1 );
    RayJson::PutVector( oNode, "edge2", m_oEdge2 );
    oNode.put( "samples", m_nSamples );
    if ( m_nAngle > 0.0 )
    {
        RayJson::PutNumber( oNode, "angle", m_nAngle );
        RayJson::PutNumber( oNode, "falloff", m_nFalloff );
    }
    OptionsToJson( oNode, m_nAngle > 0.0 );
}

BOOL CRectangleLight::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "position", "color", "edge1", "edge2", "samples", "angle", "falloff", NULL };

    if ( ! OnlyLightFields( oNode, astrFields, TRUE, TRUE ) ||
         ! RayJson::GetVector( oNode, "position", m_oPosition, TRUE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, TRUE ) ||
         ! RayJson::GetVector( oNode, "edge1", m_oEdge1, TRUE ) ||
         ! RayJson::GetVector( oNode, "edge2", m_oEdge2, TRUE ) ||
         ! RayJson::GetInt( oNode, "samples", m_nSamples, 1, MAXIMUM_LIGHT_SAMPLES, FALSE ) ||
         ! RayJson::GetNumber( oNode, "angle", m_nAngle, FALSE ) ||
         ! RayJson::GetNumber( oNode, "falloff", m_nFalloff, FALSE ) ||
         ! OptionsFromJson( oNode, TRUE, TRUE ) )
        return FALSE;
    if ( ! ( m_nAngle >= 0.0 && m_nAngle < 90.0 ) )
        return RayJson::Fail( "angle must be from 0 up to 90 degrees" );
    if ( ! ( m_nFalloff >= 0.0 && m_nFalloff <= (std::max)( m_nAngle, (FLT)0.0 ) ) )
        return RayJson::Fail( "falloff must be from 0 to the angle" );

    CVector oFacing( m_oEdge1.Cross( m_oEdge2 ) );

    if ( ! ( oFacing.Magnitude() > 0.0 ) )
        return RayJson::Fail( "edge1 and edge2 must be sides of a rectangle: not zero, and not along the same line" );
    return TRUE;
}
