#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "ReinhardColorModel.h"
#include "SrgbColorModel.h"
#include "Color.h"
#include "SceneJson.h"

#include <math.h>

CReinhardColorModel::CReinhardColorModel( void ) :
    m_nExposure( 1.0 ),
    m_nWhite( 0.0 )
{
}

BOOL CReinhardColorModel::SetExposure( FLT nExposure )
{
    if ( ! ( nExposure > 0.0 ) || ! isfinite( (double)nExposure ) )
        return FALSE;
    m_nExposure = nExposure;
    return TRUE;
}

BOOL CReinhardColorModel::SetWhite( FLT nWhite )
{
    if ( ! ( nWhite >= 0.0 ) || ! isfinite( (double)nWhite ) )
        return FALSE;
    m_nWhite = nWhite;
    return TRUE;
}

void CReinhardColorModel::ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const
{
    CColor  oSource( oColor );
    double  dRed = ( oSource.GetR() > 0.0 ) ? (double)( oSource.GetR() * m_nExposure ) : 0.0;
    double  dGreen = ( oSource.GetG() > 0.0 ) ? (double)( oSource.GetG() * m_nExposure ) : 0.0;
    double  dBlue = ( oSource.GetB() > 0.0 ) ? (double)( oSource.GetB() * m_nExposure ) : 0.0;
    double  dLuminance = ( 0.2126 * dRed ) + ( 0.7152 * dGreen ) + ( 0.0722 * dBlue );

    if ( dLuminance > 0.0 && isfinite( dLuminance ) )
    {
        double dMapped = dLuminance / ( 1.0 + dLuminance );

        if ( m_nWhite > 0.0 )
            dMapped *= 1.0 + ( dLuminance / ( (double)m_nWhite * (double)m_nWhite ) );

        double dScale = dMapped / dLuminance;

        dRed *= dScale;
        dGreen *= dScale;
        dBlue *= dScale;
    }
    else
        dRed = dGreen = dBlue = 0.0;

    nRed = ToByte( CSrgbColorModel::Encode( (FLT)dRed ) );
    nGreen = ToByte( CSrgbColorModel::Encode( (FLT)dGreen ) );
    nBlue = ToByte( CSrgbColorModel::Encode( (FLT)dBlue ) );
}

CColor CReinhardColorModel::ToLinear( const CColor &oColor ) const
{
    CColor oSource( oColor );

    return oSource * m_nExposure;
}

void CReinhardColorModel::ToJson( IASLib::CObjectNode &oNode ) const
{
    RayJson::PutNumber( oNode, "exposure", m_nExposure );
    if ( m_nWhite > 0.0 )
        RayJson::PutNumber( oNode, "white", m_nWhite );
}

BOOL CReinhardColorModel::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "exposure", "white", NULL };
    FLT nExposure = m_nExposure;
    FLT nWhite = m_nWhite;

    if ( ! oNode.isObject() )
        return TRUE;
    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetNumber( oNode, "exposure", nExposure, FALSE ) ||
         ! RayJson::GetNumber( oNode, "white", nWhite, FALSE ) )
        return FALSE;
    if ( ! SetExposure( nExposure ) )
        return RayJson::Fail( "exposure must be a positive number" );
    if ( ( oNode.get( "white" ) != NULL ) && ( ! ( nWhite > 0.0 ) || ! SetWhite( nWhite ) ) )
        return RayJson::Fail( "white must be a positive number" );
    return TRUE;
}
