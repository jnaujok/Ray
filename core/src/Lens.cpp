#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Lens.h"
#include "SceneJson.h"
#include "Threading/Mutex.h"

#include <atomic>
#include <map>

#include <math.h>
#include <stdio.h>
#include <string.h>

    // A ray must travel at least this far (mm) to reach the next surface.
#define LENS_EPSILON        1e-9

    // Height (mm) of the near-axis rays used to find the focus.
#define PARAXIAL_HEIGHT     1e-3

    // Rays across the rear element, each way, to measure the exposure.
#define EXPOSURE_GRID       128

    // The exit pupil is found for this many rings of the sensor, out to
    // PUPIL_REACH times its half width, by tracing a grid of PUPIL_GRID by
    // PUPIL_GRID points on the rear element from PUPIL_RADII points across
    // each ring.
#define PUPIL_RINGS         64
#define PUPIL_REACH         2.0
#define PUPIL_GRID          64
#define PUPIL_RADII         3
    // A ray the lens blocks is cast again, at another point in the exit
    // pupil, up to this many times in all.
#define PUPIL_TRIES         64

    // The Fraunhofer lines, in micrometers: the channels' wavelengths.
#define WAVELENGTH_C        0.65627     // red
#define WAVELENGTH_D        0.58756     // green
#define WAVELENGTH_F        0.48613     // blue

namespace
{
        // The double Gauss of US patent 2,673,491 (Tronnier): 50 mm, f/2,
        // as tabulated by Kolb, Mitchell and Hanrahan, "A Realistic Camera
        // Model for Computer Graphics" (1995), with its Abbe numbers (three
        // of its glasses are catalogue glasses: SF15 1.699/30.1, F5
        // 1.603/38.0 and LaF3 1.717/48.0). Apertures are semi-apertures.
    const CLens::SSurface s_aoDoubleGauss50[] =
    {
        {  29.475, 3.76,  1.67,  12.6,  FALSE, 47.1 },
        {  84.83,  0.12,  1.0,   12.6,  FALSE, 0.0  },
        {  19.275, 4.025, 1.67,  11.5,  FALSE, 47.1 },
        {  40.77,  3.275, 1.699, 11.5,  FALSE, 30.1 },
        {  12.75,  5.705, 1.0,   9.0,   FALSE, 0.0  },
        {   0.0,   4.5,   1.0,   8.55,  TRUE,  0.0  },
        { -14.495, 1.18,  1.603, 8.5,   FALSE, 38.0 },
        {  40.77,  6.065, 1.658, 10.0,  FALSE, 57.3 },
        { -20.385, 0.19,  1.0,   10.0,  FALSE, 0.0  },
        { 437.065, 3.22,  1.717, 10.0,  FALSE, 48.0 },
        { -39.73,  0.0,   1.0,   10.0,  FALSE, 0.0  },
    };

    const CLens::SPrescription s_aoPrescriptions[] =
    {
        { "double-gauss-50mm", "50 mm f/2 double Gauss (US 2,673,491, Tronnier), a classic standard lens",
          s_aoDoubleGauss50, (int)( sizeof( s_aoDoubleGauss50 ) / sizeof( s_aoDoubleGauss50[ 0 ] ) ) },
    };

        // Another point in the unit square for a ray's nTry-th cast, from
        // its first point: a hash of the two, so it is the same on every run
        // and in every task.
    void RetryPoint( FLT nU, FLT nV, int nTry, FLT &nNextU, FLT &nNextV )
    {
        double      dU = (double)nU;
        double      dV = (double)nV;
        uint64_t    nBitsU, nBitsV;

        memcpy( &nBitsU, &dU, sizeof( nBitsU ) );
        memcpy( &nBitsV, &dV, sizeof( nBitsV ) );

        uint64_t nKey = nBitsU ^ ( nBitsV * 0x9E3779B97F4A7C15ull ) ^ ( (uint64_t)nTry << 56 );

        for ( int nRound = 0; nRound < 2; nRound++ )
        {
            nKey += 0x9E3779B97F4A7C15ull;

            uint64_t nMix = nKey;

            nMix = ( nMix ^ ( nMix >> 30 ) ) * 0xBF58476D1CE4E5B9ull;
            nMix = ( nMix ^ ( nMix >> 27 ) ) * 0x94D049BB133111EBull;
            nMix ^= nMix >> 31;
            ( nRound == 0 ? nNextU : nNextV ) = (FLT)( (double)( nMix >> 11 ) / 9007199254740992.0 );
        }
    }

        // Shirley and Chiu's concentric map from the unit square to the unit
        // disc, which keeps an even spread even.
    void ConcentricDisc( FLT nU, FLT nV, FLT &nX, FLT &nY )
    {
        FLT nA = ( 2.0 * nU ) - 1.0;
        FLT nB = ( 2.0 * nV ) - 1.0;
        FLT nRadius = 0.0;
        FLT nAngle = 0.0;

        if ( nA != 0.0 || nB != 0.0 )
        {
            if ( ( nA * nA ) > ( nB * nB ) )
            {
                nRadius = nA;
                nAngle = ( PI / 4.0 ) * ( nB / nA );
            }
            else
            {
                nRadius = nB;
                nAngle = ( PI / 2.0 ) - ( ( PI / 4.0 ) * ( nA / nB ) );
            }
        }
        nX = nRadius * COS( nAngle );
        nY = nRadius * SIN( nAngle );
    }

    void Normalize( FLT anVector[ 3 ] )
    {
        FLT nLength = SQRT( ( anVector[ 0 ] * anVector[ 0 ] ) + ( anVector[ 1 ] * anVector[ 1 ] ) + ( anVector[ 2 ] * anVector[ 2 ] ) );

        if ( nLength > 0.0 )
        {
            anVector[ 0 ] /= nLength;
            anVector[ 1 ] /= nLength;
            anVector[ 2 ] /= nLength;
        }
    }
}

const CLens::SPrescription *CLens::GetPrescriptions( int &nCount )
{
    nCount = (int)( sizeof( s_aoPrescriptions ) / sizeof( s_aoPrescriptions[ 0 ] ) );
    return s_aoPrescriptions;
}

CLens::CLens( void ) :
    m_nFNumber( 0.0 ),
    m_nFocusDistance( 0.0 ),
    m_nSensorWidth( 36.0 ),
    m_nMillimetersPerUnit( 1000.0 ),
    m_nSeed( 0 ),
    m_bChromatic( TRUE ),
    m_nFocalLength( 0.0 ),
    m_nWideOpen( 0.0 ),
    m_nStopRadius( 0.0 ),
    m_nSensorZ( 0.0 ),
    m_nFocusedAt( 0.0 ),
    m_nExposure( 1.0 ),
    m_bPrepared( FALSE ),
    m_nPupilReach( 0.0 )
{
}

BOOL CLens::SetPrescription( const char *strName )
{
    int                     nCount;
    const SPrescription    *aoPrescriptions = GetPrescriptions( nCount );

    for ( int nIndex = 0; nIndex < nCount; nIndex++ )
    {
        if ( strcmp( aoPrescriptions[ nIndex ].strName, strName ) == 0 )
        {
            m_aoSurfaces.assign( aoPrescriptions[ nIndex ].aoSurfaces,
                                 aoPrescriptions[ nIndex ].aoSurfaces + aoPrescriptions[ nIndex ].nSurfaces );
            m_strPrescription = strName;
            m_bPrepared = FALSE;
            return TRUE;
        }
    }
    return FALSE;
}

void CLens::SetSurfaces( const std::vector<SSurface> &aoSurfaces )
{
    m_aoSurfaces = aoSurfaces;
    m_strPrescription.clear();
    m_bPrepared = FALSE;
}

FLT CLens::IorAt( int nIndex, int nChannel ) const
{
    if ( nIndex < 0 )
        return 1.0;

    const SSurface &oSurface = m_aoSurfaces[ (size_t)nIndex ];

    if ( nChannel == CHANNEL_ALL || nChannel == CHANNEL_GREEN || ! m_bChromatic ||
         ! ( oSurface.nAbbe > 0.0 ) || oSurface.nIor == 1.0 )
        return oSurface.nIor;

        // Cauchy's n = A + B / wavelength^2, through n at the d line and the
        // Abbe number V = ( n_d - 1 ) / ( n_F - n_C ).
    FLT nB = ( oSurface.nIor - 1.0 ) /
             ( oSurface.nAbbe * ( ( 1.0 / ( WAVELENGTH_F * WAVELENGTH_F ) ) - ( 1.0 / ( WAVELENGTH_C * WAVELENGTH_C ) ) ) );
    FLT nA = oSurface.nIor - ( nB / ( WAVELENGTH_D * WAVELENGTH_D ) );
    FLT nWavelength = ( nChannel == CHANNEL_RED ) ? WAVELENGTH_C : WAVELENGTH_F;

    return nA + ( nB / ( nWavelength * nWavelength ) );
}

BOOL CLens::IsChromatic( void ) const
{
    if ( ! m_bChromatic )
        return FALSE;
    for ( size_t nIndex = 0; nIndex < m_aoSurfaces.size(); nIndex++ )
    {
        if ( m_aoSurfaces[ nIndex ].nAbbe > 0.0 && m_aoSurfaces[ nIndex ].nIor != 1.0 )
            return TRUE;
    }
    return FALSE;
}

FLT CLens::FocalLengthIn( int nChannel ) const
{
    FLT anOrigin[ 3 ] = { 0.0, PARAXIAL_HEIGHT, -1.0 };
    FLT anDirection[ 3 ] = { 0.0, 0.0, 1.0 };

    if ( ! m_bPrepared || ! Trace( anOrigin, anDirection, TRUE, 1e300, nChannel ) || anDirection[ 1 ] >= 0.0 )
        return 0.0;
    return -PARAXIAL_HEIGHT / ( anDirection[ 1 ] / anDirection[ 2 ] );
}

BOOL CLens::Trace( FLT anOrigin[ 3 ], FLT anDirection[ 3 ], BOOL bForward, FLT nStopRadius, int nChannel ) const
{
    int nCount = (int)m_aoSurfaces.size();

    for ( int nStep = 0; nStep < nCount; nStep++ )
    {
        int             nIndex = bForward ? nStep : ( nCount - 1 - nStep );
        const SSurface &oSurface = m_aoSurfaces[ (size_t)nIndex ];
        FLT             nBefore = IorAt( nIndex - 1, nChannel );
        FLT             nAfter = IorAt( nIndex, nChannel );
        FLT             nVertex = m_anVertexZ[ (size_t)nIndex ];
        FLT             anHit[ 3 ];
        FLT             anNormal[ 3 ];

        if ( oSurface.nRadius == 0.0 )
        {
                // A flat surface.
            if ( anDirection[ 2 ] == 0.0 )
                return FALSE;

            FLT nT = ( nVertex - anOrigin[ 2 ] ) / anDirection[ 2 ];

            if ( nT <= LENS_EPSILON )
                return FALSE;
            for ( int nAxis = 0; nAxis < 3; nAxis++ )
                anHit[ nAxis ] = anOrigin[ nAxis ] + ( nT * anDirection[ nAxis ] );
            anNormal[ 0 ] = 0.0;
            anNormal[ 1 ] = 0.0;
            anNormal[ 2 ] = -1.0;
        }
        else
        {
                // A sphere centered on the axis. Of the two places the ray
                // meets it, the surface is the one on the vertex's side of
                // the center.
            FLT anCenter[ 3 ] = { 0.0, 0.0, nVertex + oSurface.nRadius };
            FLT anFromCenter[ 3 ] = { anOrigin[ 0 ] - anCenter[ 0 ], anOrigin[ 1 ] - anCenter[ 1 ], anOrigin[ 2 ] - anCenter[ 2 ] };
            FLT nB = ( anFromCenter[ 0 ] * anDirection[ 0 ] ) + ( anFromCenter[ 1 ] * anDirection[ 1 ] ) + ( anFromCenter[ 2 ] * anDirection[ 2 ] );
            FLT nC = ( anFromCenter[ 0 ] * anFromCenter[ 0 ] ) + ( anFromCenter[ 1 ] * anFromCenter[ 1 ] ) +
                     ( anFromCenter[ 2 ] * anFromCenter[ 2 ] ) - ( oSurface.nRadius * oSurface.nRadius );
            FLT nDiscriminant = ( nB * nB ) - nC;
            BOOL bFound = FALSE;

            if ( nDiscriminant < 0.0 )
                return FALSE;

            FLT anRoots[ 2 ] = { -nB - SQRT( nDiscriminant ), -nB + SQRT( nDiscriminant ) };

            for ( int nRoot = 0; nRoot < 2 && ! bFound; nRoot++ )
            {
                FLT nT = anRoots[ nRoot ];

                if ( nT <= LENS_EPSILON )
                    continue;
                for ( int nAxis = 0; nAxis < 3; nAxis++ )
                    anHit[ nAxis ] = anOrigin[ nAxis ] + ( nT * anDirection[ nAxis ] );
                if ( ( anHit[ 2 ] - anCenter[ 2 ] ) * oSurface.nRadius < 0.0 )
                    bFound = TRUE;
            }
            if ( ! bFound )
                return FALSE;

                // Points toward the scene at the vertex, whatever the sign
                // of the radius.
            for ( int nAxis = 0; nAxis < 3; nAxis++ )
                anNormal[ nAxis ] = ( anHit[ nAxis ] - anCenter[ nAxis ] ) / oSurface.nRadius;
        }

            // Past the surface's edge (or the stop's opening) is the barrel.
        FLT nLimit = oSurface.bStop ? nStopRadius : oSurface.nAperture;

        if ( ( ( anHit[ 0 ] * anHit[ 0 ] ) + ( anHit[ 1 ] * anHit[ 1 ] ) ) > ( nLimit * nLimit ) )
            return FALSE;

            // Snell's law, with the normal turned against the ray.
        FLT nCosIn = -( ( anNormal[ 0 ] * anDirection[ 0 ] ) + ( anNormal[ 1 ] * anDirection[ 1 ] ) + ( anNormal[ 2 ] * anDirection[ 2 ] ) );

        if ( nCosIn < 0.0 )
        {
            for ( int nAxis = 0; nAxis < 3; nAxis++ )
                anNormal[ nAxis ] = -anNormal[ nAxis ];
            nCosIn = -nCosIn;
        }

        FLT nEta = bForward ? ( nBefore / nAfter ) : ( nAfter / nBefore );
        FLT nSin2Out = nEta * nEta * ( 1.0 - ( nCosIn * nCosIn ) );

        if ( nSin2Out > 1.0 )
            return FALSE;       // totally reflected inside the glass

        FLT nCosOut = SQRT( 1.0 - nSin2Out );

        for ( int nAxis = 0; nAxis < 3; nAxis++ )
        {
            anDirection[ nAxis ] = ( nEta * anDirection[ nAxis ] ) + ( ( ( nEta * nCosIn ) - nCosOut ) * anNormal[ nAxis ] );
            anOrigin[ nAxis ] = anHit[ nAxis ];
        }
        Normalize( anDirection );
    }
    return TRUE;
}

BOOL CLens::ImageZ( FLT nObjectZ, BOOL bInfinity, FLT &nImageZ, int nChannel ) const
{
    FLT anOrigin[ 3 ];
    FLT anDirection[ 3 ];
    FLT nFullStop = 0.0;

    for ( size_t nIndex = 0; nIndex < m_aoSurfaces.size(); nIndex++ )
    {
        if ( m_aoSurfaces[ nIndex ].bStop )
            nFullStop = m_aoSurfaces[ nIndex ].nAperture;
    }

    if ( bInfinity )
    {
        anOrigin[ 0 ] = 0.0;
        anOrigin[ 1 ] = PARAXIAL_HEIGHT;
        anOrigin[ 2 ] = -1.0;
        anDirection[ 0 ] = 0.0;
        anDirection[ 1 ] = 0.0;
        anDirection[ 2 ] = 1.0;
    }
    else
    {
            // The ray from the object point to height PARAXIAL_HEIGHT at the
            // front surface, started 1 mm in front of the lens rather than at
            // a distant object, where the sphere tests would lose precision.
        FLT nDistance = -nObjectZ;

        anOrigin[ 0 ] = 0.0;
        if ( nDistance > 1.0 )
        {
            anOrigin[ 1 ] = PARAXIAL_HEIGHT * ( nDistance - 1.0 ) / nDistance;
            anOrigin[ 2 ] = -1.0;
        }
        else
        {
            anOrigin[ 1 ] = 0.0;
            anOrigin[ 2 ] = nObjectZ;
        }
        anDirection[ 0 ] = 0.0;
        anDirection[ 1 ] = PARAXIAL_HEIGHT;
        anDirection[ 2 ] = nDistance;
        Normalize( anDirection );
    }

    if ( ! Trace( anOrigin, anDirection, TRUE, nFullStop, nChannel ) || anDirection[ 1 ] == 0.0 )
        return FALSE;

    nImageZ = anOrigin[ 2 ] + ( ( -anOrigin[ 1 ] / anDirection[ 1 ] ) * anDirection[ 2 ] );
    return TRUE;
}

FLT CLens::GetFilmDistance( void ) const
{
    return m_anVertexZ.empty() ? 0.0 : m_nSensorZ - m_anVertexZ.back();
}

BOOL CLens::Prepare( FLT nDefaultFocus, std::string &strError )
{
    char    achBuffer[ 256 ];
    int     nStops = 0;
    FLT     nFullStop = 0.0;

    m_bPrepared = FALSE;
    if ( m_aoSurfaces.empty() )
    {
        strError = "a lens needs at least one surface";
        return FALSE;
    }

    m_anVertexZ.assign( m_aoSurfaces.size(), 0.0 );
    for ( size_t nIndex = 1; nIndex < m_aoSurfaces.size(); nIndex++ )
        m_anVertexZ[ nIndex ] = m_anVertexZ[ nIndex - 1 ] + m_aoSurfaces[ nIndex - 1 ].nThickness;
    for ( size_t nIndex = 0; nIndex < m_aoSurfaces.size(); nIndex++ )
    {
        if ( m_aoSurfaces[ nIndex ].bStop )
        {
            nStops++;
            nFullStop = m_aoSurfaces[ nIndex ].nAperture;
        }
    }

        // The focal length, from a ray coming in parallel near the axis.
    {
        FLT anOrigin[ 3 ] = { 0.0, PARAXIAL_HEIGHT, -1.0 };
        FLT anDirection[ 3 ] = { 0.0, 0.0, 1.0 };

        if ( ! Trace( anOrigin, anDirection, TRUE, nFullStop ) || anDirection[ 1 ] >= 0.0 )
        {
            strError = "the lens doesn't bring light to a focus behind it (it isn't a converging lens)";
            return FALSE;
        }
        m_nFocalLength = -PARAXIAL_HEIGHT / ( anDirection[ 1 ] / anDirection[ 2 ] );
    }

        // Wide open: the entrance pupil is the widest beam of parallel rays
        // the lens lets through.
    {
        FLT nLow = 0.0;
        FLT nHigh = m_aoSurfaces[ 0 ].nAperture;

        for ( int nStep = 0; nStep < 60; nStep++ )
        {
            FLT nMiddle = ( nLow + nHigh ) / 2.0;
            FLT anOrigin[ 3 ] = { 0.0, nMiddle, -1.0 };
            FLT anDirection[ 3 ] = { 0.0, 0.0, 1.0 };

            if ( Trace( anOrigin, anDirection, TRUE, nFullStop ) )
                nLow = nMiddle;
            else
                nHigh = nMiddle;
        }
        if ( nLow <= 0.0 )
        {
            strError = "no light gets through the lens";
            return FALSE;
        }
        m_nWideOpen = m_nFocalLength / ( 2.0 * nLow );
    }

    if ( m_nFNumber > 0.0 )
    {
        if ( nStops == 0 )
        {
            strError = "an f-number needs a surface marked as the aperture stop";
            return FALSE;
        }
        if ( m_nFNumber < m_nWideOpen - 1e-9 )
        {
            snprintf( achBuffer, sizeof( achBuffer ), "the lens opens only to f/%.2f", (double)m_nWideOpen );
            strError = achBuffer;
            return FALSE;
        }
            // The entrance pupil grows in step with the stop's opening.
        m_nStopRadius = nFullStop * ( m_nWideOpen / m_nFNumber );
    }
    else
        m_nStopRadius = nFullStop;

        // Focus: put the sensor where the focus distance is imaged.
    m_nFocusedAt = ( m_nFocusDistance > 0.0 ) ? m_nFocusDistance : nDefaultFocus;
    if ( ! ( m_nFocusedAt > 0.0 ) || ! ImageZ( -m_nFocusedAt * m_nMillimetersPerUnit, FALSE, m_nSensorZ ) ||
         ! ( m_nSensorZ > m_anVertexZ.back() ) )
    {
        snprintf( achBuffer, sizeof( achBuffer ), "the lens can't focus at %g scene units (%g mm)",
                  (double)m_nFocusedAt, (double)( m_nFocusedAt * m_nMillimetersPerUnit ) );
        strError = achBuffer;
        return FALSE;
    }

        // Exposure: the share of rays from the sensor's center to the rear
        // element that get through.
    {
        FLT nRear = m_aoSurfaces.back().nAperture;
        int nTried = 0;
        int nPassed = 0;

        for ( int nRow = 0; nRow < EXPOSURE_GRID; nRow++ )
        {
            for ( int nColumn = 0; nColumn < EXPOSURE_GRID; nColumn++ )
            {
                FLT nX = ( ( ( nColumn + 0.5 ) / EXPOSURE_GRID ) * 2.0 - 1.0 ) * nRear;
                FLT nY = ( ( ( nRow + 0.5 ) / EXPOSURE_GRID ) * 2.0 - 1.0 ) * nRear;

                if ( ( nX * nX ) + ( nY * nY ) > nRear * nRear )
                    continue;

                FLT anOrigin[ 3 ] = { 0.0, 0.0, m_nSensorZ };
                FLT anDirection[ 3 ] = { nX, nY, m_anVertexZ.back() - m_nSensorZ };

                Normalize( anDirection );
                nTried++;
                if ( Trace( anOrigin, anDirection, FALSE, m_nStopRadius ) )
                    nPassed++;
            }
        }
        if ( nPassed == 0 )
        {
            strError = "no light reaches the center of the sensor";
            return FALSE;
        }
        m_nExposure = (FLT)nTried / (FLT)nPassed;
    }

        // The exit pupil's table, shared with any lens set up the same way.
    m_nPupilReach = ( m_nSensorWidth / 2.0 ) * PUPIL_REACH;
    m_pPupil = SharedPupilTable( *this );
    m_bPrepared = TRUE;
    return TRUE;
}

void CLens::FindPupil( int nRing, SPupil &oPupil ) const
{
    FLT                 nRear = m_aoSurfaces.back().nAperture;
    FLT                 nCell = ( 2.0 * nRear ) / PUPIL_GRID;
    int                 anChannels[ 3 ] = { CHANNEL_ALL, CHANNEL_ALL, CHANNEL_ALL };
    int                 nChannels = 1;
    std::vector<FLT>    anPassed;       // x, y of each grid point that got through

        // A chromatic lens's pupil is where any of its colors gets through.
    if ( IsChromatic() )
    {
        anChannels[ 0 ] = CHANNEL_RED;
        anChannels[ 1 ] = CHANNEL_GREEN;
        anChannels[ 2 ] = CHANNEL_BLUE;
        nChannels = 3;
    }

    {
        FLT     nMinX = 1e300, nMaxX = -1e300, nMinY = 1e300, nMaxY = -1e300;

        anPassed.clear();
        for ( int nRow = 0; nRow < PUPIL_GRID; nRow++ )
        {
            for ( int nColumn = 0; nColumn < PUPIL_GRID; nColumn++ )
            {
                FLT nX = ( ( nColumn + 0.5 ) * nCell ) - nRear;
                FLT nY = ( ( nRow + 0.5 ) * nCell ) - nRear;
                BOOL bPassed = FALSE;

                if ( ( nX * nX ) + ( nY * nY ) > nRear * nRear )
                    continue;

                    // From points across the ring, in any channel.
                for ( int nRadius = 0; ! bPassed && nRadius < PUPIL_RADII; nRadius++ )
                {
                    FLT nSensorX = ( ( nRing + ( (FLT)nRadius / ( PUPIL_RADII - 1 ) ) ) / PUPIL_RINGS ) * m_nPupilReach;

                    for ( int nChannel = 0; ! bPassed && nChannel < nChannels; nChannel++ )
                    {
                        FLT anOrigin[ 3 ] = { nSensorX, 0.0, m_nSensorZ };
                        FLT anDirection[ 3 ] = { nX - nSensorX, nY, m_anVertexZ.back() - m_nSensorZ };

                        Normalize( anDirection );
                        bPassed = Trace( anOrigin, anDirection, FALSE, m_nStopRadius, anChannels[ nChannel ] );
                    }
                }
                if ( bPassed )
                {
                    anPassed.push_back( nX );
                    anPassed.push_back( nY );
                    nMinX = ( nX < nMinX ) ? nX : nMinX;
                    nMaxX = ( nX > nMaxX ) ? nX : nMaxX;
                    nMinY = ( nY < nMinY ) ? nY : nMinY;
                    nMaxY = ( nY > nMaxY ) ? nY : nMaxY;
                }
            }
        }

        oPupil.bOpen = anPassed.empty() ? FALSE : TRUE;
        if ( ! oPupil.bOpen )
            return;

            // An ellipse in the passing points' bounding box, grown until it
            // holds the whole cell around every one of them, and a little
            // more, so nothing that gets through is left out.
        oPupil.nCenterX = ( nMinX + nMaxX ) / 2.0;
        oPupil.nCenterY = ( nMinY + nMaxY ) / 2.0;
        oPupil.nHalfX = ( ( nMaxX - nMinX ) / 2.0 ) + ( nCell / 2.0 );
        oPupil.nHalfY = ( ( nMaxY - nMinY ) / 2.0 ) + ( nCell / 2.0 );

        FLT nFarthest = 0.0;

        for ( size_t nAt = 0; nAt < anPassed.size(); nAt += 2 )
        {
            FLT nDX = anPassed[ nAt ] - oPupil.nCenterX;
            FLT nDY = anPassed[ nAt + 1 ] - oPupil.nCenterY;

                // The cell's corner farthest from the center.
            nDX = ( nDX >= 0.0 ) ? nDX + ( nCell / 2.0 ) : nDX - ( nCell / 2.0 );
            nDY = ( nDY >= 0.0 ) ? nDY + ( nCell / 2.0 ) : nDY - ( nCell / 2.0 );

            FLT nReach = ( ( nDX * nDX ) / ( oPupil.nHalfX * oPupil.nHalfX ) ) + ( ( nDY * nDY ) / ( oPupil.nHalfY * oPupil.nHalfY ) );

            nFarthest = ( nReach > nFarthest ) ? nReach : nFarthest;
        }

        FLT nGrow = ( nFarthest > 1.0 ) ? SQRT( nFarthest ) : 1.0;

        oPupil.nHalfX *= nGrow * 1.01;
        oPupil.nHalfY *= nGrow * 1.01;

            // No smaller than the whole rear element (light gets through all
            // of it, as when the stop is the last surface): aim at that.
        if ( oPupil.nHalfX * oPupil.nHalfY >= nRear * nRear )
        {
            oPupil.nCenterX = oPupil.nCenterY = 0.0;
            oPupil.nHalfX = oPupil.nHalfY = nRear;
        }
    }
}

struct CLens::SPupilTable
{
    SPupil                                  aoRings[ PUPIL_RINGS ];
    std::atomic<bool>                       abDone[ PUPIL_RINGS ];
    IASLib::CMutex                          amutexRings[ PUPIL_RINGS ];

        // The share of rays that get through, at each ring's inner edge
        // (and the last ring's outer edge), for each channel.
    FLT                                     anPass[ 4 ][ PUPIL_RINGS + 1 ];
    std::atomic<bool>                       abPassDone[ 4 ][ PUPIL_RINGS + 1 ];
    IASLib::CMutex                          amutexPass[ 4 ][ PUPIL_RINGS + 1 ];

    SPupilTable( void )
    {
        for ( int nRing = 0; nRing < PUPIL_RINGS; nRing++ )
            abDone[ nRing ] = false;
        for ( int nChannel = 0; nChannel < 4; nChannel++ )
        {
            for ( int nKnot = 0; nKnot <= PUPIL_RINGS; nKnot++ )
                abPassDone[ nChannel ][ nKnot ] = false;
        }
    }
};

namespace
{
        // What a lens's exit pupil depends on, as bytes.
    std::string PupilKey( const std::vector<CLens::SSurface> &aoSurfaces, const std::vector<FLT> &anVertexZ, FLT nStopRadius,
                          FLT nSensorZ, FLT nSensorWidth, BOOL bChromatic )
    {
        std::string strKey;
        auto        Add = [ &strKey ]( FLT nValue ) { strKey.append( (const char *)&nValue, sizeof( nValue ) ); };

        for ( size_t nIndex = 0; nIndex < aoSurfaces.size(); nIndex++ )
        {
            Add( aoSurfaces[ nIndex ].nRadius );
            Add( aoSurfaces[ nIndex ].nThickness );
            Add( aoSurfaces[ nIndex ].nIor );
            Add( aoSurfaces[ nIndex ].nAperture );
            Add( aoSurfaces[ nIndex ].bStop ? 1.0 : 0.0 );
            Add( aoSurfaces[ nIndex ].nAbbe );
        }
        for ( size_t nIndex = 0; nIndex < anVertexZ.size(); nIndex++ )
            Add( anVertexZ[ nIndex ] );
        Add( nStopRadius );
        Add( nSensorZ );
        Add( nSensorWidth );
        Add( bChromatic ? 1.0 : 0.0 );
        return strKey;
    }
}

std::shared_ptr<CLens::SPupilTable> CLens::SharedPupilTable( const CLens &oLens )
{
    static IASLib::CMutex                                       s_mutex;
    static std::map<std::string, std::weak_ptr<SPupilTable>>    s_mapTables;

    std::string                     strKey = PupilKey( oLens.m_aoSurfaces, oLens.m_anVertexZ, oLens.m_nStopRadius, oLens.m_nSensorZ,
                                                       oLens.m_nSensorWidth, oLens.IsChromatic() );
    IASLib::CMutexLock              oLock( s_mutex );
    std::shared_ptr<SPupilTable>    pTable = s_mapTables[ strKey ].lock();

    if ( ! pTable )
    {
        pTable = std::make_shared<SPupilTable>();
        s_mapTables[ strKey ] = pTable;
    }
    return pTable;
}

const CLens::SPupil &CLens::PupilFor( int nRing ) const
{
    SPupilTable &oTable = *m_pPupil;

    if ( ! oTable.abDone[ nRing ].load( std::memory_order_acquire ) )
    {
        IASLib::CMutexLock oLock( oTable.amutexRings[ nRing ] );

        if ( ! oTable.abDone[ nRing ].load( std::memory_order_relaxed ) )
        {
            FindPupil( nRing, oTable.aoRings[ nRing ] );
            oTable.abDone[ nRing ].store( true, std::memory_order_release );
        }
    }
    return oTable.aoRings[ nRing ];
}

FLT CLens::PassFor( int nKnot, int nChannel ) const
{
    SPupilTable &oTable = *m_pPupil;

    if ( ! oTable.abPassDone[ nChannel ][ nKnot ].load( std::memory_order_acquire ) )
    {
        IASLib::CMutexLock oLock( oTable.amutexPass[ nChannel ][ nKnot ] );

        if ( ! oTable.abPassDone[ nChannel ][ nKnot ].load( std::memory_order_relaxed ) )
        {
            oTable.anPass[ nChannel ][ nKnot ] = FindPass( nKnot, nChannel );
            oTable.abPassDone[ nChannel ][ nKnot ].store( true, std::memory_order_release );
        }
    }
    return oTable.anPass[ nChannel ][ nKnot ];
}

FLT CLens::FindPass( int nKnot, int nChannel ) const
{
    FLT nRear = m_aoSurfaces.back().nAperture;
    FLT nSensorX = ( (FLT)nKnot / PUPIL_RINGS ) * m_nPupilReach;
    int nTried = 0;
    int nPassed = 0;

        // The same grid the exposure is measured on, so at the center this
        // is exactly the share the exposure makes up for.
    for ( int nRow = 0; nRow < EXPOSURE_GRID; nRow++ )
    {
        for ( int nColumn = 0; nColumn < EXPOSURE_GRID; nColumn++ )
        {
            FLT nX = ( ( ( nColumn + 0.5 ) / EXPOSURE_GRID ) * 2.0 - 1.0 ) * nRear;
            FLT nY = ( ( ( nRow + 0.5 ) / EXPOSURE_GRID ) * 2.0 - 1.0 ) * nRear;

            if ( ( nX * nX ) + ( nY * nY ) > nRear * nRear )
                continue;

            FLT anOrigin[ 3 ] = { nSensorX, 0.0, m_nSensorZ };
            FLT anDirection[ 3 ] = { nX - nSensorX, nY, m_anVertexZ.back() - m_nSensorZ };

            Normalize( anDirection );
            nTried++;
            if ( Trace( anOrigin, anDirection, FALSE, m_nStopRadius, nChannel ) )
                nPassed++;
        }
    }
    return nTried ? (FLT)nPassed / (FLT)nTried : 0.0;
}

BOOL CLens::RayFor( FLT nScreenX, FLT nScreenY, FLT nAspect, FLT nLensU, FLT nLensV, FLT anOrigin[ 3 ], FLT anDirection[ 3 ],
                    int nChannel, FLT *pnWeight ) const
{
    if ( pnWeight )
        *pnWeight = 1.0;
    if ( ! m_bPrepared || ! ( nAspect > 0.0 ) )
        return FALSE;

    FLT nHalfWidth = m_nSensorWidth / 2.0;
    FLT nHalfHeight = nHalfWidth / nAspect;
    FLT nRear = m_aoSurfaces.back().nAperture;
    FLT nX = 0.0;
    FLT nY = 0.0;

        // The image lands upside down and mirrored on the sensor.
    anOrigin[ 0 ] = -nScreenX * nHalfWidth;
    anOrigin[ 1 ] = -nScreenY * nHalfHeight;
    anOrigin[ 2 ] = m_nSensorZ;

    FLT nFromCenter = SQRT( ( anOrigin[ 0 ] * anOrigin[ 0 ] ) + ( anOrigin[ 1 ] * anOrigin[ 1 ] ) );
    int nRing = ( m_nPupilReach > 0.0 ) ? (int)( ( nFromCenter / m_nPupilReach ) * PUPIL_RINGS ) : PUPIL_RINGS;

    if ( nRing < 0 || nRing >= PUPIL_RINGS || ! m_pPupil )
    {
            // Beyond the rings: anywhere on the rear element, by Shirley and
            // Chiu's concentric map from the unit square to the disc, which
            // keeps an even spread even.
        ConcentricDisc( nLensU, nLensV, nX, nY );
        nX *= nRear;
        nY *= nRear;
    }
    else
    {
            // Inside the exit pupil's ellipse for this ring, found for a
            // point on the +x axis, turned to where this point is. A ray the
            // lens blocks is cast again at another point, until one gets
            // through; it counts for the share of rays aimed anywhere on the
            // rear element that would have, so the image is exactly as bright
            // (and vignetted) as before, without the noise of blocked rays.
        const SPupil &oPupil = PupilFor( nRing );

        if ( ! oPupil.bOpen )
            return FALSE;

        FLT     nCos = ( nFromCenter > 0.0 ) ? anOrigin[ 0 ] / nFromCenter : 1.0;
        FLT     nSin = ( nFromCenter > 0.0 ) ? anOrigin[ 1 ] / nFromCenter : 0.0;
        FLT     nU = nLensU;
        FLT     nV = nLensV;
        FLT     anStart[ 3 ] = { anOrigin[ 0 ], anOrigin[ 1 ], anOrigin[ 2 ] };

        for ( int nTry = 0; nTry < PUPIL_TRIES; nTry++ )
        {
            FLT nDiscX, nDiscY;

            if ( nTry > 0 )
                RetryPoint( nLensU, nLensV, nTry, nU, nV );
            ConcentricDisc( nU, nV, nDiscX, nDiscY );

            FLT nPupilX = oPupil.nCenterX + ( nDiscX * oPupil.nHalfX );
            FLT nPupilY = oPupil.nCenterY + ( nDiscY * oPupil.nHalfY );

            nX = ( nPupilX * nCos ) - ( nPupilY * nSin );
            nY = ( nPupilX * nSin ) + ( nPupilY * nCos );
            if ( ( nX * nX ) + ( nY * nY ) > nRear * nRear )
                continue;       // off the rear element

            anOrigin[ 0 ] = anStart[ 0 ];
            anOrigin[ 1 ] = anStart[ 1 ];
            anOrigin[ 2 ] = anStart[ 2 ];
            anDirection[ 0 ] = nX - anOrigin[ 0 ];
            anDirection[ 1 ] = nY - anOrigin[ 1 ];
            anDirection[ 2 ] = m_anVertexZ.back() - anOrigin[ 2 ];
            Normalize( anDirection );
            if ( Trace( anOrigin, anDirection, FALSE, m_nStopRadius, nChannel ) )
            {
                if ( pnWeight )
                {
                        // The share that gets through, between this ring's
                        // edges.
                    FLT nAcross = ( ( nFromCenter / m_nPupilReach ) * PUPIL_RINGS ) - nRing;
                    int nSlot = ( nChannel >= CHANNEL_RED && nChannel <= CHANNEL_BLUE ) ? nChannel : CHANNEL_ALL;

                    *pnWeight = ( PassFor( nRing, nSlot ) * ( 1.0 - nAcross ) ) + ( PassFor( nRing + 1, nSlot ) * nAcross );
                }
                return TRUE;
            }
        }
        return FALSE;       // nothing got through: black
    }

    anDirection[ 0 ] = nX - anOrigin[ 0 ];
    anDirection[ 1 ] = nY - anOrigin[ 1 ];
    anDirection[ 2 ] = m_anVertexZ.back() - anOrigin[ 2 ];
    Normalize( anDirection );

    return Trace( anOrigin, anDirection, FALSE, m_nStopRadius, nChannel );
}

std::string CLens::Describe( void ) const
{
    char achBuffer[ 512 ];

    snprintf( achBuffer, sizeof( achBuffer ),
              "%s: %.1f mm, f/%.2f (opens to f/%.2f), focused at %g scene units, sensor %.1f mm wide, %.2f mm behind the rear surface",
              m_strPrescription.empty() ? "lens" : m_strPrescription.c_str(), (double)m_nFocalLength, (double)GetFNumber(),
              (double)m_nWideOpen, (double)m_nFocusedAt, (double)m_nSensorWidth, (double)GetFilmDistance() );

    std::string strDescription = achBuffer;

    if ( IsChromatic() )
    {
        snprintf( achBuffer, sizeof( achBuffer ), "; chromatic: focal length %.3f mm red, %.3f green, %.3f blue",
                  (double)FocalLengthIn( CHANNEL_RED ), (double)FocalLengthIn( CHANNEL_GREEN ), (double)FocalLengthIn( CHANNEL_BLUE ) );
        strDescription += achBuffer;
    }
    return strDescription;
}

void CLens::ToJson( IASLib::CObjectNode &oNode ) const
{
    if ( ! m_strPrescription.empty() )
        oNode.put( "prescription", m_strPrescription );
    else
    {
        IASLib::CArrayNode *pSurfaces = oNode.putArray( "surfaces" );

        for ( size_t nIndex = 0; nIndex < m_aoSurfaces.size(); nIndex++ )
        {
            const SSurface         &oSurface = m_aoSurfaces[ nIndex ];
            IASLib::CObjectNode    *pSurface = pSurfaces->addObject();

            RayJson::PutNumber( *pSurface, "radius", oSurface.nRadius );
            RayJson::PutNumber( *pSurface, "thickness", oSurface.nThickness );
            RayJson::PutNumber( *pSurface, "ior", oSurface.nIor );
            RayJson::PutNumber( *pSurface, "aperture", oSurface.nAperture );
            if ( oSurface.bStop )
                RayJson::PutBool( *pSurface, "stop", TRUE );
            if ( oSurface.nAbbe > 0.0 )
                RayJson::PutNumber( *pSurface, "abbe", oSurface.nAbbe );
        }
    }
    if ( m_nFNumber > 0.0 )
        RayJson::PutNumber( oNode, "fNumber", m_nFNumber );
    if ( m_nFocusDistance > 0.0 )
        RayJson::PutNumber( oNode, "focusDistance", m_nFocusDistance );
    RayJson::PutNumber( oNode, "sensorWidth", m_nSensorWidth );
    RayJson::PutNumber( oNode, "millimetersPerUnit", m_nMillimetersPerUnit );
    oNode.put( "seed", m_nSeed );
    if ( ! m_bChromatic )
        RayJson::PutBool( oNode, "chromatic", FALSE );
}

BOOL CLens::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "prescription", "surfaces", "fNumber", "focusDistance", "sensorWidth",
                                              "millimetersPerUnit", "seed", "chromatic", NULL };
    static const char *const astrSurface[] = { "radius", "thickness", "ior", "aperture", "stop", "abbe", NULL };
    std::string             strPrescription;

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetString( oNode, "prescription", strPrescription, FALSE ) ||
         ! RayJson::GetNumber( oNode, "fNumber", m_nFNumber, FALSE ) ||
         ! RayJson::GetNumber( oNode, "focusDistance", m_nFocusDistance, FALSE ) ||
         ! RayJson::GetNumber( oNode, "sensorWidth", m_nSensorWidth, FALSE ) ||
         ! RayJson::GetNumber( oNode, "millimetersPerUnit", m_nMillimetersPerUnit, FALSE ) ||
         ! RayJson::GetInt( oNode, "seed", m_nSeed, 0, 2147483647, FALSE ) ||
         ! RayJson::GetBool( oNode, "chromatic", m_bChromatic, FALSE ) )
        return FALSE;

    if ( m_nFNumber < 0.0 )
        return RayJson::Fail( "fNumber must be positive" );
    if ( m_nFocusDistance < 0.0 )
        return RayJson::Fail( "focusDistance must be positive" );
    if ( ! ( m_nSensorWidth > 0.0 ) )
        return RayJson::Fail( "sensorWidth must be positive" );
    if ( ! ( m_nMillimetersPerUnit > 0.0 ) )
        return RayJson::Fail( "millimetersPerUnit must be positive" );

    const IASLib::CJsonNode *pSurfaces = oNode.get( "surfaces" );

    if ( ( pSurfaces != NULL ) == ! strPrescription.empty() )
        return RayJson::Fail( "a lens needs a \"prescription\" (a built-in lens's name) or its \"surfaces\", but not both" );

    if ( ! strPrescription.empty() )
    {
        if ( ! SetPrescription( strPrescription.c_str() ) )
            return RayJson::Fail( "unknown lens prescription \"" + strPrescription + "\"" );
        return TRUE;
    }

    RayJson::CWhere         oWhere( "surfaces" );
    std::vector<SSurface>   aoSurfaces;
    int                     nStops = 0;

    if ( ! pSurfaces->isArray() || pSurfaces->size() == 0 )
        return RayJson::Fail( "must be an array of surfaces, front to back" );
    for ( int nIndex = 0; nIndex < pSurfaces->size(); nIndex++ )
    {
        RayJson::CWhere             oElement( "", nIndex );
        const IASLib::CJsonNode    &oSurfaceNode = *pSurfaces->get( nIndex );
        SSurface                    oSurface = { 0.0, 0.0, 1.0, 0.0, FALSE, 0.0 };

        if ( ! RayJson::OnlyFields( oSurfaceNode, astrSurface ) ||
             ! RayJson::GetNumber( oSurfaceNode, "radius", oSurface.nRadius, TRUE ) ||
             ! RayJson::GetNumber( oSurfaceNode, "thickness", oSurface.nThickness, TRUE ) ||
             ! RayJson::GetNumber( oSurfaceNode, "ior", oSurface.nIor, FALSE ) ||
             ! RayJson::GetNumber( oSurfaceNode, "aperture", oSurface.nAperture, TRUE ) ||
             ! RayJson::GetBool( oSurfaceNode, "stop", oSurface.bStop, FALSE ) ||
             ! RayJson::GetNumber( oSurfaceNode, "abbe", oSurface.nAbbe, FALSE ) )
            return FALSE;
        if ( oSurface.nAbbe < 0.0 )
            return RayJson::Fail( "abbe can't be negative" );
        if ( oSurface.nThickness < 0.0 )
            return RayJson::Fail( "thickness can't be negative" );
        if ( ! ( oSurface.nIor >= 1.0 ) )
            return RayJson::Fail( "ior must be at least 1" );
        if ( ! ( oSurface.nAperture > 0.0 ) )
            return RayJson::Fail( "aperture (the surface's radius in mm) must be positive" );
        if ( oSurface.bStop && oSurface.nRadius != 0.0 )
            return RayJson::Fail( "the aperture stop must be flat (radius 0)" );
        if ( oSurface.nRadius != 0.0 && ABS( oSurface.nRadius ) < oSurface.nAperture )
            return RayJson::Fail( "the surface is wider than its sphere (|radius| must be at least the aperture)" );
        if ( oSurface.bStop && ++nStops > 1 )
            return RayJson::Fail( "a lens has only one aperture stop" );
        aoSurfaces.push_back( oSurface );
    }
    SetSurfaces( aoSurfaces );
    return TRUE;
}
