#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "SrgbColorModel.h"
#include "Color.h"

#include <math.h>

FLT CSrgbColorModel::Encode( FLT nLinear )
{
    double dLinear = (double)Clamp( nLinear );

    if ( dLinear <= 0.0031308 )
        return (FLT)( dLinear * 12.92 );
    return (FLT)( ( 1.055 * pow( dLinear, 1.0 / 2.4 ) ) - 0.055 );
}

void CSrgbColorModel::ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const
{
    CColor  oSource( oColor );

    nRed = ToByte( Encode( oSource.GetR() ) );
    nGreen = ToByte( Encode( oSource.GetG() ) );
    nBlue = ToByte( Encode( oSource.GetB() ) );
}
