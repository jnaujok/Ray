#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "GammaColorModel.h"
#include "Color.h"
#include "SceneJson.h"

#include <math.h>

CGammaColorModel::CGammaColorModel( FLT nGamma ) :
    m_nGamma( 2.2 )
{
    SetGamma( nGamma );
}

BOOL CGammaColorModel::SetGamma( FLT nGamma )
{
    if ( ! ( nGamma > 0.0 ) || ! isfinite( (double)nGamma ) )
        return FALSE;
    m_nGamma = nGamma;
    return TRUE;
}

void CGammaColorModel::ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const
{
    CColor  oSource( oColor );
    double  dPower = 1.0 / (double)m_nGamma;

    nRed = ToByte( pow( (double)Clamp( oSource.GetR() ), dPower ) );
    nGreen = ToByte( pow( (double)Clamp( oSource.GetG() ), dPower ) );
    nBlue = ToByte( pow( (double)Clamp( oSource.GetB() ), dPower ) );
}

void CGammaColorModel::ToJson( IASLib::CObjectNode &oNode ) const
{
    RayJson::PutNumber( oNode, "gamma", m_nGamma );
}

BOOL CGammaColorModel::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "gamma", NULL };
    FLT nGamma = m_nGamma;

    if ( ! oNode.isObject() )
        return TRUE;
    if ( ! RayJson::OnlyFields( oNode, astrFields ) || ! RayJson::GetNumber( oNode, "gamma", nGamma, FALSE ) )
        return FALSE;
    return SetGamma( nGamma ) ? TRUE : RayJson::Fail( "gamma must be a positive number" );
}
