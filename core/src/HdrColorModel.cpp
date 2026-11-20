/*************************************************************************
*   CHdrColorModel                                                       *
*                                                                        *
*   High dynamic range output. See HdrColorModel.h.                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "HdrColorModel.h"
#include "SrgbColorModel.h"
#include "Color.h"
#include "SceneJson.h"

#include <math.h>

CHdrColorModel::CHdrColorModel( void ) :
    m_nExposure( 1.0 )
{
}

BOOL CHdrColorModel::SetExposure( FLT nExposure )
{
    if ( ! ( nExposure > 0.0 ) || ! isfinite( (double)nExposure ) )
        return FALSE;
    m_nExposure = nExposure;
    return TRUE;
}

CColor CHdrColorModel::ToLinear( const CColor &oColor ) const
{
    CColor oSource( oColor );

    return oSource * m_nExposure;
}

void CHdrColorModel::ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const
{
    CColor oScaled = ToLinear( oColor );

    nRed = ToByte( CSrgbColorModel::Encode( oScaled.GetR() ) );
    nGreen = ToByte( CSrgbColorModel::Encode( oScaled.GetG() ) );
    nBlue = ToByte( CSrgbColorModel::Encode( oScaled.GetB() ) );
}

void CHdrColorModel::ToJson( IASLib::CObjectNode &oNode ) const
{
    RayJson::PutNumber( oNode, "exposure", m_nExposure );
}

BOOL CHdrColorModel::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "exposure", NULL };
    FLT nExposure = m_nExposure;

    if ( ! oNode.isObject() )
        return TRUE;
    if ( ! RayJson::OnlyFields( oNode, astrFields ) || ! RayJson::GetNumber( oNode, "exposure", nExposure, FALSE ) )
        return FALSE;
    if ( ! SetExposure( nExposure ) )
        return RayJson::Fail( "exposure must be a positive number" );
    return TRUE;
}
