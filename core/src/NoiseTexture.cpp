#include "Globals.h"

#include "NoiseTexture.h"

namespace
{
    const FLT s_anGray[] =
    {
        0.0, 0.0, 0.0,
        1.0, 1.0, 1.0,
    };
}

CNoiseTexture::CNoiseTexture( void ) :
    CSolidTexture( s_anGray, 2, 0.0 )
{
}

FLT CNoiseTexture::Pattern( FLT nX, FLT nY, FLT nZ ) const
{
    if ( m_nTurbulence > 0.0 )
        return m_nTurbulence * Turbulence( nX, nY, nZ );

    FLT nSum = 0.0;
    FLT nSize = 1.0;

    for ( int nOctave = 0; nOctave < m_nOctaves; nOctave++ )
    {
        nSum += Noise( nX * nSize, nY * nSize, nZ * nSize ) / nSize;
        nSize *= 2.0;
    }
    return 0.5 + ( 0.5 * nSum );
}
