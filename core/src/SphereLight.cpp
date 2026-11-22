/*************************************************************************
*   CSphereLight                                                         *
*                                                                        *
*   A glowing ball: an area light that casts soft shadows. See           *
*   SphereLight.h.                                                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "SphereLight.h"
#include "Sampling.h"
#include "SceneJson.h"


#define MAXIMUM_LIGHT_SAMPLES   256

CSphereLight::CSphereLight( void ) :
m_nRadius( 1.0 ),
m_nSamples( 1 )
{
    m_oColor.Set( 1.0, 1.0, 1.0 );
}

CSphereLight::~CSphereLight( void )
{
}

CColor CSphereLight::GetShade( CVector oVector )
{
    (void)oVector;
    return GetColor();
}

BOOL CSphereLight::SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint )
{
        // A point on the disc the ball shows oFrom: through its center, at
        // right angles to the line of sight.
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


DYNAMIC_IMPLEMENTS( CSphereLight, "sphereLight" );

void CSphereLight::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "position", m_oPosition );
    RayJson::PutColor( oNode, "color", m_oColor );
    RayJson::PutNumber( oNode, "radius", m_nRadius );
    oNode.put( "samples", m_nSamples );
    OptionsToJson( oNode, FALSE );
}

BOOL CSphereLight::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "position", "color", "radius", "samples", NULL };

    if ( ! OnlyLightFields( oNode, astrFields, TRUE, FALSE ) ||
         ! RayJson::GetVector( oNode, "position", m_oPosition, TRUE ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, TRUE ) ||
         ! RayJson::GetNumber( oNode, "radius", m_nRadius, FALSE ) ||
         ! RayJson::GetInt( oNode, "samples", m_nSamples, 1, MAXIMUM_LIGHT_SAMPLES, FALSE ) ||
         ! OptionsFromJson( oNode, TRUE, FALSE ) )
        return FALSE;
    if ( ! ( m_nRadius > 0.0 ) )
        return RayJson::Fail( "radius must be more than 0" );
    return TRUE;
}
