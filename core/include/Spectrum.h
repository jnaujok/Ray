/*************************************************************************
*   RaySpectrum                                                          *
*                                                                        *
*   Light as a spectrum, for dispersion traced wavelength by wavelength  *
*   (a camera's "dispersion": "spectral"). Where white light would split *
*   into colors (dispersive glass, a chromatic lens), it splits instead  *
*   into four rays at single wavelengths, one from each quarter of the   *
*   visible range, all at the same place in their quarters: where comes  *
*   from which of its pixel's samples the ray is                         *
*   (CRay::WavelengthOffset), so a pixel's samples take one each of as   *
*   many                                                                 *
*   strata. What each sees is weighed by how a wavelength looks: the CIE *
*   1931 color matching functions (Wyman, Sloan and Shirley's 2013 fit), *
*   turned to linear sRGB, and scaled so an even spread of wavelengths   *
*   adds up to exactly white. Over a pixel's samples the wavelengths     *
*   spread evenly over the range, so a prism fans light into a smooth    *
*   rainbow instead of three bars.                                       *
*                                                                        *
*   sRGB can't show the purest spectral colors, so the weights go below  *
*   zero in places (red, between about 440 and 540 nm); averaged over a  *
*   pixel they come out right, and the color model clips what is left.   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __SPECTRUM_H__
#define __SPECTRUM_H__

#include "Globals.h"

namespace RaySpectrum
{
        // The range traced, in nanometers.
    const FLT   SHORTEST = 380.0;
    const FLT   LONGEST = 720.0;
        // How many rays a split makes: one in each part of the range. Four,
        // all at the same place in their parts, give a far smoother result
        // than three (a quarter of three's grain in red, a fifth in green)
        // for a third more rays.
    const int   PARTS = 4;

        /// How a wavelength (in nanometers) looks, as linear red, green and
        /// blue, scaled so their average over the range is 1 each.
    void        Weights( FLT nNanometers, FLT anRGB[ 3 ] );
        /// The wavelength of part nPart (0 to PARTS - 1) of a split, at
        /// nWhere (0 up to 1) evenly across that part.
    FLT         PartWavelength( int nPart, FLT nWhere );
        /// The wavelength (in nanometers) part nPart of a split takes at
        /// nWhere (0 up to 1), and its weights.
    void        PartWeights( int nPart, FLT nWhere, FLT &nNanometers, FLT anRGB[ 3 ] );
}

#endif // __SPECTRUM_H__
