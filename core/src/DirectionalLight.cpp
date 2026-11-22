/*************************************************************************
*   CDirectionalLight                                                    *
*                                                                        *
*   Sunlight, from one direction. See DirectionalLight.h.                *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "DirectionalLight.h"
#include "Sampling.h"
#include "SceneJson.h"

#include <math.h>


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
    OptionsToJson( oNode, FALSE );
}

void CDirectionalLight::FilterAt( const CVector &oPoint, const CVector &oToLight, STexturePoint &oAt ) const
{
    CVector oDirection( m_oDirection );
    CVector oFirst;
    CVector oSecond;
    CVector oAt3( oPoint );

    (void)oToLight;
    oDirection.Normalize();
    RaySampling::Basis( oDirection, oFirst, oSecond );

    FLT     nAlong = oDirection.Dot( oAt3 );
    CVector oAlong( oDirection * nAlong );
    CVector oAcross( oAt3 - oAlong );

    oAt.oPoint = oAcross * ( 1.0 / m_nFilterSize );
    oAt.nU = ( oFirst.Dot( oAt3 ) / m_nFilterSize ) + 0.5;
    oAt.nV = ( oSecond.Dot( oAt3 ) / m_nFilterSize ) + 0.5;
    oAt.bHasUV = TRUE;
}

BOOL CDirectionalLight::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "direction", "color", "angle", "samples", NULL };

    if ( ! OnlyLightFields( oNode, astrFields, FALSE, FALSE ) ||
         ! RayJson::GetVector( oNode, "direction", m_oDirection, TRUE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, TRUE ) ||
         ! RayJson::GetNumber( oNode, "angle", m_nAngle, FALSE ) ||
         ! RayJson::GetInt( oNode, "samples", m_nSamples, 1, MAXIMUM_LIGHT_SAMPLES, FALSE ) ||
         ! OptionsFromJson( oNode, FALSE, FALSE ) )
        return FALSE;
    if ( m_oDirection.Magnitude() == 0.0 )
        return RayJson::Fail( "direction must not be [0, 0, 0]" );
    if ( ! ( m_nAngle >= 0.0 && m_nAngle < 90.0 ) )
        return RayJson::Fail( "angle must be from 0 up to 90 degrees" );
    return TRUE;
}
