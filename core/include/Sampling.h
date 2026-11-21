/*************************************************************************
*   Sampling                                                             *
*                                                                        *
*   Helpers for the effects that trace a ray (or a shadow ray) in one of *
*   many directions: rough glass and mirrors, and area lights. Nothing   *
*   here is random: each ray carries a sample point and a key            *
*   (CRay::SamplePoint), worked out from where its primary ray fell in   *
*   its pixel, so the same scene always gives the same image, and        *
*   networked engines give exactly what threads do.                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __SAMPLING_H__
#define __SAMPLING_H__

#include "Globals.h"
#include "Vector.h"

#include <math.h>
#include <stdint.h>

namespace RaySampling
{
        // What a sample is for, so each use gets its own shift of the
        // ray's sample point.
    enum EPurpose
    {
        PURPOSE_REFLECT = 0x5245464CULL,        // a rough mirror's direction
        PURPOSE_REFRACT = 0x52465241ULL,        // rough glass's direction
        PURPOSE_LIGHT = 0x4C494748ULL,          // plus the light's index: a point on an area light
        PURPOSE_ENVIRONMENT = 0x454E5649ULL,    // a direction toward the sky, for the environment's light
        PURPOSE_WAVELENGTH = 0x57415645ULL,     // the wavelengths white light splits into (spectral dispersion)
        PURPOSE_INDIRECT = 0x494E4449ULL        // a direction to gather bounced light from (indirect light)
    };

        /// SplitMix64's finalizer.
    inline uint64_t Mix( uint64_t nValue )
    {
        nValue += 0x9E3779B97F4A7C15ULL;
        nValue = ( nValue ^ ( nValue >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
        nValue = ( nValue ^ ( nValue >> 27 ) ) * 0x94D049BB133111EBULL;
        return nValue ^ ( nValue >> 31 );
    }

        /// A number from 0 up to 1 from a hash's top bits.
    inline FLT Unit( uint64_t nValue )
    {
        return (FLT)( (double)( nValue >> 11 ) * ( 1.0 / 9007199254740992.0 ) );
    }

        /// Where index nIndex (0 up to nCount) goes in a shuffle of 0 up to
        /// nCount picked by nSeed: Kensler's stateless permutation
        /// ("Correlated Multi-Jittered Sampling", 2013). Every index goes
        /// to a different place, and the same seed always gives the same
        /// shuffle.
    inline uint32_t Permute( uint32_t nIndex, uint32_t nCount, uint32_t nSeed )
    {
        uint32_t nMask = nCount - 1;

        nMask |= nMask >> 1;
        nMask |= nMask >> 2;
        nMask |= nMask >> 4;
        nMask |= nMask >> 8;
        nMask |= nMask >> 16;
        do
        {
            nIndex ^= nSeed;
            nIndex *= 0xe170893dU;
            nIndex ^= nSeed >> 16;
            nIndex ^= ( nIndex & nMask ) >> 4;
            nIndex ^= nSeed >> 8;
            nIndex *= 0x0929eb3fU;
            nIndex ^= nSeed >> 23;
            nIndex ^= ( nIndex & nMask ) >> 1;
            nIndex *= 1U | ( nSeed >> 27 );
            nIndex *= 0x6935fa69U;
            nIndex ^= ( nIndex & nMask ) >> 11;
            nIndex *= 0x74dcb303U;
            nIndex ^= ( nIndex & nMask ) >> 2;
            nIndex *= 0x9e501cc3U;
            nIndex ^= ( nIndex & nMask ) >> 2;
            nIndex *= 0xc860a3dfU;
            nIndex &= nMask;
            nIndex ^= nIndex >> 5;
        } while ( nIndex >= nCount );
        return ( nIndex + nSeed ) % nCount;
    }

        /// The fractional part, from 0 up to 1.
    inline FLT Wrap( FLT nValue )
    {
        return nValue - (FLT)floor( (double)nValue );
    }

        /// The kth of several points spread over the unit square from
        /// (nU, nV): Roberts's R2 sequence, shifted to start there, so a few
        /// points cover it evenly and k = 0 is (nU, nV) itself.
    inline void Spread( FLT nU, FLT nV, int nK, FLT &nOutU, FLT &nOutV )
    {
        nOutU = Wrap( nU + ( nK * 0.7548776662466927 ) );
        nOutV = Wrap( nV + ( nK * 0.5698402909980532 ) );
    }

        /// Shirley and Chiu's concentric map from the unit square to the
        /// unit disc, which keeps evenly spread points evenly spread.
    inline void ConcentricDisc( FLT nU, FLT nV, FLT &nX, FLT &nY )
    {
        FLT nA = ( 2.0 * nU ) - 1.0;
        FLT nB = ( 2.0 * nV ) - 1.0;
        FLT nRadius;
        FLT nAngle;

        if ( nA == 0.0 && nB == 0.0 )
        {
            nX = nY = 0.0;
            return;
        }
        if ( ABS( nA ) > ABS( nB ) )
        {
            nRadius = nA;
            nAngle = ( PI / 4.0 ) * ( nB / nA );
        }
        else
        {
            nRadius = nB;
            nAngle = ( PI / 2.0 ) - ( ( PI / 4.0 ) * ( nA / nB ) );
        }
        nX = nRadius * (FLT)cos( (double)nAngle );
        nY = nRadius * (FLT)sin( (double)nAngle );
    }

        /// Two unit vectors at right angles to the unit vector oAxis and to
        /// each other.
    inline void Basis( CVector oAxis, CVector &oFirst, CVector &oSecond )
    {
        CVector oHelper = ( ABS( oAxis.GetX() ) < 0.9 ) ? CVector( 1.0, 0.0, 0.0 ) : CVector( 0.0, 1.0, 0.0 );

        oFirst = oAxis.Cross( oHelper );
        oFirst.Normalize();
        oSecond = oAxis.Cross( oFirst );
        oSecond.Normalize();
    }

        /// A microfacet normal about the unit normal oNormal, drawn from the
        /// GGX (Trowbridge-Reitz) distribution of roughness nAlpha from the
        /// point (nU, nV) of the unit square: tilted from the normal by
        /// atan( alpha sqrt( u / ( 1 - u ) ) ), turned 2 pi v round it.
    inline CVector Microfacet( CVector oNormal, FLT nAlpha, FLT nU, FLT nV )
    {
        CVector oFirst;
        CVector oSecond;

        if ( nU > 0.999999 )
            nU = 0.999999;

        FLT     nTan = nAlpha * SQRT( nU / ( 1.0 - nU ) );
        FLT     nCos = 1.0 / SQRT( 1.0 + ( nTan * nTan ) );
        FLT     nSin = nTan * nCos;
        FLT     nPhi = 2.0 * PI * nV;

        Basis( oNormal, oFirst, oSecond );

        CVector oAlong = oNormal * nCos;
        CVector oAcross1 = oFirst * ( nSin * (FLT)cos( (double)nPhi ) );
        CVector oAcross2 = oSecond * ( nSin * (FLT)sin( (double)nPhi ) );
        CVector oSum = oAlong + oAcross1;
        CVector oResult = oSum + oAcross2;

        oResult.Normalize();
        return oResult;
    }
}

#endif // __SAMPLING_H__
