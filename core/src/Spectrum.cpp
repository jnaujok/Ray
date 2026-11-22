/*************************************************************************
*   RaySpectrum                                                          *
*                                                                        *
*   See Spectrum.h.                                                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#include "Globals.h"

#include "Spectrum.h"

#include <math.h>

namespace
{
        // One lobe of the fit: a Gaussian with a different width on each
        // side of its peak.
    double Lobe( double dWavelength, double dPeak, double dBelow, double dAbove )
    {
        double dSpread = ( dWavelength - dPeak ) / ( ( dWavelength < dPeak ) ? dBelow : dAbove );

        return exp( -0.5 * dSpread * dSpread );
    }

        // The CIE 1931 2-degree color matching functions, by Wyman, Sloan and
        // Shirley's multi-lobe fit ("Simple Analytic Approximations to the
        // CIE XYZ Color Matching Functions", 2013), turned to linear sRGB.
    void LinearRGB( double dWavelength, double adRGB[ 3 ] )
    {
        double dX = ( 1.056 * Lobe( dWavelength, 599.8, 37.9, 31.0 ) ) + ( 0.362 * Lobe( dWavelength, 442.0, 16.0, 26.7 ) ) -
                    ( 0.065 * Lobe( dWavelength, 501.1, 20.4, 26.2 ) );
        double dY = ( 0.821 * Lobe( dWavelength, 568.8, 46.9, 40.5 ) ) + ( 0.286 * Lobe( dWavelength, 530.9, 16.3, 31.1 ) );
        double dZ = ( 1.217 * Lobe( dWavelength, 437.0, 11.8, 36.0 ) ) + ( 0.681 * Lobe( dWavelength, 459.0, 26.0, 13.8 ) );

        adRGB[ 0 ] = ( 3.2406 * dX ) - ( 1.5372 * dY ) - ( 0.4986 * dZ );
        adRGB[ 1 ] = ( -0.9689 * dX ) + ( 1.8758 * dY ) + ( 0.0415 * dZ );
        adRGB[ 2 ] = ( 0.0557 * dX ) - ( 0.2040 * dY ) + ( 1.0570 * dZ );
    }

        // The average of each over the range, found once by summing it at
        // the middles of many small steps.
    struct SAverage
    {
        double  adAverage[ 3 ];

        SAverage( void )
        {
            const int   STEPS = 34000;
            double      dStep = ( (double)RaySpectrum::LONGEST - (double)RaySpectrum::SHORTEST ) / STEPS;

            adAverage[ 0 ] = adAverage[ 1 ] = adAverage[ 2 ] = 0.0;
            for ( int nStep = 0; nStep < STEPS; nStep++ )
            {
                double adRGB[ 3 ];

                LinearRGB( (double)RaySpectrum::SHORTEST + ( ( nStep + 0.5 ) * dStep ), adRGB );
                for ( int nIndex = 0; nIndex < 3; nIndex++ )
                    adAverage[ nIndex ] += adRGB[ nIndex ];
            }
            for ( int nIndex = 0; nIndex < 3; nIndex++ )
                adAverage[ nIndex ] /= STEPS;
        }
    };

    const SAverage &Average( void )
    {
        static const SAverage oAverage;

        return oAverage;
    }
}

void RaySpectrum::Weights( FLT nNanometers, FLT anRGB[ 3 ] )
{
    const SAverage &oAverage = Average();
    double          adRGB[ 3 ];

    LinearRGB( (double)nNanometers, adRGB );
    for ( int nIndex = 0; nIndex < 3; nIndex++ )
        anRGB[ nIndex ] = (FLT)( adRGB[ nIndex ] / oAverage.adAverage[ nIndex ] );
}

void RaySpectrum::PartWeights( int nPart, FLT nWhere, FLT &nNanometers, FLT anRGB[ 3 ] )
{
    nNanometers = PartWavelength( nPart, nWhere );
    Weights( nNanometers, anRGB );
}

FLT RaySpectrum::PartWavelength( int nPart, FLT nWhere )
{
    return SHORTEST + ( ( ( nPart + nWhere ) / PARTS ) * ( LONGEST - SHORTEST ) );
}
