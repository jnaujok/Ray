#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Haze.h"
#include "SceneJson.h"

#include <math.h>

CHaze::CHaze( void ) :
m_oColor( 0.7, 0.75, 0.8 )
{
    m_nDensity = 0.0;
    m_nHeightFalloff = 0.0;
    m_nBaseHeight = 0.0;
}

FLT CHaze::OpticalDepth( const CVector &oOrigin, const CVector &oDirection, FLT nLength ) const
{
    if ( m_nDensity <= 0.0 || ! ( nLength > 0.0 ) )
        return 0.0;
    if ( m_nHeightFalloff <= 0.0 )
        return m_nDensity * nLength;

        // density( t ) = d exp( -k ( y0 + t dy - base ) ), so its integral
        // from 0 to L is d exp( -k ( y0 - base ) ) ( 1 - exp( -k dy L ) ) / ( k dy ).
    CVector oFrom( oOrigin );
    CVector oAlong( oDirection );
    double  dHeight = (double)oFrom.GetY() - (double)m_nBaseHeight;
    double  dLift = - (double)m_nHeightFalloff * dHeight;
    double  dRate = (double)m_nHeightFalloff * (double)oAlong.GetY();
    double  dExponent = dRate * (double)nLength;

    if ( dLift > 700.0 )
        return (FLT)1e300;              // far down in ever thicker haze: nothing gets through
    double  dStart = (double)m_nDensity * exp( dLift );

    if ( ABS( dExponent ) < 1e-12 )
        return (FLT)( dStart * (double)nLength );
    if ( dExponent < -700.0 )
        return (FLT)1e300;
    return (FLT)( dStart * ( -expm1( -dExponent ) ) / dRate );
}

FLT CHaze::Transmittance( const CVector &oOrigin, const CVector &oDirection, FLT nLength ) const
{
    FLT nDepth = OpticalDepth( oOrigin, oDirection, nLength );

    if ( nDepth <= 0.0 )
        return 1.0;
    if ( nDepth > 700.0 )
        return 0.0;
    return (FLT)exp( - (double)nDepth );
}

CColor CHaze::Through( const CColor &oColor, FLT nTransmittance ) const
{
    CColor  oSeen( oColor );
    CColor  oHaze( m_oColor );
    CColor  oNear( oSeen * nTransmittance );
    CColor  oFar( oHaze * ( 1.0 - nTransmittance ) );

    return oNear + oFar;
}

void CHaze::ToJson( IASLib::CObjectNode &oNode ) const
{
    RayJson::PutColor( oNode, "color", m_oColor );
    RayJson::PutNumber( oNode, "density", m_nDensity );
    if ( m_nHeightFalloff != 0.0 )
    {
        RayJson::PutNumber( oNode, "heightFalloff", m_nHeightFalloff );
        RayJson::PutNumber( oNode, "baseHeight", m_nBaseHeight );
    }
}

BOOL CHaze::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "color", "density", "heightFalloff", "baseHeight", NULL };

    if ( ! oNode.isObject() )
        return RayJson::Fail( "must be an object" );
    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, FALSE ) ||
         ! RayJson::GetNumber( oNode, "density", m_nDensity, TRUE ) ||
         ! RayJson::GetNumber( oNode, "heightFalloff", m_nHeightFalloff, FALSE ) ||
         ! RayJson::GetNumber( oNode, "baseHeight", m_nBaseHeight, FALSE ) )
        return FALSE;
    if ( ! ( m_nDensity >= 0.0 ) || m_nDensity > 1e6 )
        return RayJson::Fail( "density must be from 0 to 1000000" );
    if ( ! ( m_nHeightFalloff >= 0.0 ) || m_nHeightFalloff > 1e6 )
        return RayJson::Fail( "heightFalloff must be from 0 to 1000000" );
    if ( ! ( ABS( m_nBaseHeight ) < 1e12 ) )
        return RayJson::Fail( "baseHeight is out of range" );
    return TRUE;
}
