#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "Lens.h"
#include "SceneJson.h"
#include "Threading/Thread.h"
#include "JSON/ObjectNode.h"

#include <atomic>
#include <functional>
#include <thread>

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

        // The Apollo Hasselblads' Carl Zeiss lenses, at their nominal focal
        // lengths, written by tools/apollo_lenses.py (which says where each
        // comes from): patent examples where Zeiss published one, and
        // reconstructions where it didn't. Glasses are (nd, Vd), or fused
        // silica and calcium fluoride by their Sellmeier formulas.
    // BEGIN APOLLO LENSES (tools/apollo_lenses.py --write)
        // Zeiss Biogon 38 mm f/4.5 (US 2,721,499 Ex. 2, Bertele): the SWC's lens (patent example).
    const CLens::SSurface s_aoBiogon38mm[] =
    {
        {    41.4500,   1.4052, 1.50380,  20.557, FALSE, 66.70, CLens::GLASS_NONE },
        {    19.9882,   5.0132, 1.00000,  16.693, FALSE,  0.00, CLens::GLASS_NONE },
        {    41.8715,   1.4052, 1.48697,  16.651, FALSE, 70.30, CLens::GLASS_NONE },
        {    19.2628,  13.2925, 1.00000,  14.321, FALSE,  0.00, CLens::GLASS_NONE },
        {    21.3592,  11.1278, 1.72050,  10.418, FALSE, 50.30, CLens::GLASS_NONE },
        {     9.6352,   5.0512, 1.60739,   5.720, FALSE, 59.50, CLens::GLASS_NONE },
        {   -74.0281,   0.8925, 1.00000,   5.605, FALSE,  0.00, CLens::GLASS_NONE },
        {     0.0000,   0.8925, 1.00000,   5.276, TRUE ,  0.00, CLens::GLASS_NONE },
        {   -95.9722,   1.0634, 1.56993,   5.485, FALSE, 57.50, CLens::GLASS_NONE },
        {    11.6177,   9.0010, 1.62500,   5.433, FALSE, 53.30, CLens::GLASS_NONE },
        {    -9.6884,   6.9121, 1.71966,   7.017, FALSE, 29.30, CLens::GLASS_NONE },
        {   -21.7922,  14.8117, 1.00000,  10.124, FALSE,  0.00, CLens::GLASS_NONE },
        {   -15.2484,   3.7219, 1.64200,  13.615, FALSE, 58.10, CLens::GLASS_NONE },
        {   -38.9813,   0.0000, 1.00000,  18.730, FALSE,  0.00, CLens::GLASS_NONE },
    };

        // Zeiss Biogon 60 mm f/5.6 (HDC): a reconstruction from the Biogon of US 2,721,499 (reconstruction).
    const CLens::SSurface s_aoBiogon60mm[] =
    {
        {    66.6472,   2.2594, 1.50380,  23.804, FALSE, 66.70, CLens::GLASS_NONE },
        {    32.1389,   8.0607, 1.00000,  20.874, FALSE,  0.00, CLens::GLASS_NONE },
        {    67.3250,   2.2594, 1.48697,  19.316, FALSE, 70.30, CLens::GLASS_NONE },
        {    30.9726,  21.3730, 1.00000,  17.512, FALSE,  0.00, CLens::GLASS_NONE },
        {    34.3434,  17.8923, 1.72050,  12.590, FALSE, 50.30, CLens::GLASS_NONE },
        {    15.4924,   8.1217, 1.60739,   7.278, FALSE, 59.50, CLens::GLASS_NONE },
        {  -119.0294,   1.4350, 1.00000,   7.210, FALSE,  0.00, CLens::GLASS_NONE },
        {     0.0000,   1.4350, 1.00000,   6.817, TRUE ,  0.00, CLens::GLASS_NONE },
        {  -154.3132,   1.7098, 1.56993,   7.052, FALSE, 57.50, CLens::GLASS_NONE },
        {    18.6800,  14.4726, 1.62500,   6.996, FALSE, 53.30, CLens::GLASS_NONE },
        {   -15.5779,  11.1140, 1.71966,   8.791, FALSE, 29.30, CLens::GLASS_NONE },
        {   -35.0395,  23.8156, 1.00000,  12.185, FALSE,  0.00, CLens::GLASS_NONE },
        {   -24.5179,   5.9844, 1.64200,  16.936, FALSE, 58.10, CLens::GLASS_NONE },
        {   -62.6779,   0.0000, 1.00000,  21.142, FALSE,  0.00, CLens::GLASS_NONE },
    };

        // Zeiss Biogon 60 mm f/5.6 with the Hasselblad Data Camera's Reseau plate (3 mm, assumed BK7) (reconstruction).
    const CLens::SSurface s_aoBiogon60mmReseau[] =
    {
        {    66.6472,   2.2594, 1.50380,  23.804, FALSE, 66.70, CLens::GLASS_NONE },
        {    32.1389,   8.0607, 1.00000,  20.874, FALSE,  0.00, CLens::GLASS_NONE },
        {    67.3250,   2.2594, 1.48697,  19.316, FALSE, 70.30, CLens::GLASS_NONE },
        {    30.9726,  21.3730, 1.00000,  17.512, FALSE,  0.00, CLens::GLASS_NONE },
        {    34.3434,  17.8923, 1.72050,  12.590, FALSE, 50.30, CLens::GLASS_NONE },
        {    15.4924,   8.1217, 1.60739,   7.278, FALSE, 59.50, CLens::GLASS_NONE },
        {  -119.0294,   1.4350, 1.00000,   7.210, FALSE,  0.00, CLens::GLASS_NONE },
        {     0.0000,   1.4350, 1.00000,   6.817, TRUE ,  0.00, CLens::GLASS_NONE },
        {  -154.3132,   1.7098, 1.56993,   7.052, FALSE, 57.50, CLens::GLASS_NONE },
        {    18.6800,  14.4726, 1.62500,   6.996, FALSE, 53.30, CLens::GLASS_NONE },
        {   -15.5779,  11.1140, 1.71966,   8.791, FALSE, 29.30, CLens::GLASS_NONE },
        {   -35.0395,  23.8156, 1.00000,  12.185, FALSE,  0.00, CLens::GLASS_NONE },
        {   -24.5179,   5.9844, 1.64200,  16.936, FALSE, 58.10, CLens::GLASS_NONE },
        {   -62.6779,  28.1642, 1.00000,  21.142, FALSE,  0.00, CLens::GLASS_NONE },
        {     0.0000,   3.0000, 1.51680,  38.932, FALSE, 64.17, CLens::GLASS_NONE },
        {     0.0000,   0.0000, 1.00000,  39.963, FALSE,  0.00, CLens::GLASS_NONE },
    };

        // Zeiss Planar 80 mm f/2.8 (US 2,724,994 Ex. I, Lange): 5 elements, 4 groups (patent example).
    const CLens::SSurface s_aoPlanar80mm[] =
    {
        {    35.4269,   1.4976, 1.62536,  18.885, FALSE, 35.60, CLens::GLASS_NONE },
        {    21.4086,   6.2904, 1.62041,  16.785, FALSE, 60.30, CLens::GLASS_NONE },
        {    77.0610,   0.1000, 1.00000,  16.492, FALSE,  0.00, CLens::GLASS_NONE },
        {    23.3390,   9.3455, 1.76182,  14.173, FALSE, 26.50, CLens::GLASS_NONE },
        {    15.1559,   5.0160, 1.00000,  10.192, FALSE,  0.00, CLens::GLASS_NONE },
        {     0.0000,  11.4631, 1.00000,   9.721, TRUE ,  0.00, CLens::GLASS_NONE },
        {   -14.3079,   2.0968, 1.76182,  11.509, FALSE, 26.50, CLens::GLASS_NONE },
        {   -18.8071,   0.1000, 1.00000,  13.591, FALSE,  0.00, CLens::GLASS_NONE },
        {   -90.2793,   6.2304, 1.62041,  17.546, FALSE, 60.30, CLens::GLASS_NONE },
        {   -22.3526,   0.0000, 1.00000,  17.570, FALSE,  0.00, CLens::GLASS_NONE },
    };

        // Zeiss Planar 100 mm f/3.5: the Planar of US 2,724,994 Ex. II at 100 mm and f/3.5 (patent family).
    const CLens::SSurface s_aoPlanar100mm[] =
    {
        {    44.3409,   2.6240, 1.69895,  19.468, FALSE, 30.10, CLens::GLASS_NONE },
        {    29.2120,   6.9980, 1.69347,  17.740, FALSE, 53.50, CLens::GLASS_NONE },
        {    84.7329,   0.1250, 1.00000,  16.461, FALSE,  0.00, CLens::GLASS_NONE },
        {    29.6350,  11.5970, 1.69895,  14.968, FALSE, 30.10, CLens::GLASS_NONE },
        {    18.9690,   6.2700, 1.00000,  10.358, FALSE,  0.00, CLens::GLASS_NONE },
        {     0.0000,  14.2250, 1.00000,   9.805, TRUE ,  0.00, CLens::GLASS_NONE },
        {   -17.9080,   2.4370, 1.76182,  12.294, FALSE, 26.50, CLens::GLASS_NONE },
        {   -23.5390,   0.2500, 1.00000,  14.174, FALSE,  0.00, CLens::GLASS_NONE },
        {  -111.3798,   7.5360, 1.62041,  16.744, FALSE, 60.30, CLens::GLASS_NONE },
        {   -27.5770,   0.0000, 1.00000,  17.975, FALSE,  0.00, CLens::GLASS_NONE },
    };

        // Zeiss UV-Sonnar 105 mm f/4.3: a reconstruction, 7 air-spaced silica and CaF2 singlets (reconstruction).
    const CLens::SSurface s_aoUvSonnar105mm[] =
    {
        {    56.4181,   5.5762, 1.43385,  20.569, FALSE,  0.00, CLens::GLASS_CALCIUM_FLUORIDE },
        { -13971.7251,   0.4182, 1.00000,  19.823, FALSE,  0.00, CLens::GLASS_NONE },
        {    35.3194,   5.5762, 1.43385,  17.120, FALSE,  0.00, CLens::GLASS_CALCIUM_FLUORIDE },
        {   148.0516,   0.8364, 1.00000,  15.809, FALSE,  0.00, CLens::GLASS_NONE },
        {   351.9545,   2.2305, 1.45846,  15.484, FALSE,  0.00, CLens::GLASS_FUSED_SILICA },
        {    25.8695,   2.7882, 1.00000,  12.776, FALSE,  0.00, CLens::GLASS_NONE },
        {    43.2072,   4.1823, 1.43385,  12.101, FALSE,  0.00, CLens::GLASS_CALCIUM_FLUORIDE },
        {    98.9418,   8.3644, 1.00000,  10.692, FALSE,  0.00, CLens::GLASS_NONE },
        {     0.0000,   8.3644, 1.00000,   9.031, TRUE ,  0.00, CLens::GLASS_NONE },
        {   -33.2809,   1.9517, 1.45846,   9.125, FALSE,  0.00, CLens::GLASS_FUSED_SILICA },
        {    72.3483,   1.1152, 1.00000,  10.723, FALSE,  0.00, CLens::GLASS_NONE },
        {   141.4637,   4.4610, 1.43385,  11.386, FALSE,  0.00, CLens::GLASS_CALCIUM_FLUORIDE },
        {   -35.0826,   0.4182, 1.00000,  12.296, FALSE,  0.00, CLens::GLASS_NONE },
        {  -314.4022,   3.3457, 1.43385,  13.406, FALSE,  0.00, CLens::GLASS_CALCIUM_FLUORIDE },
        {   -54.6954,   0.0000, 1.00000,  14.065, FALSE,  0.00, CLens::GLASS_NONE },
    };

        // Zeiss Sonnar 250 mm f/5.6: a reconstruction, 4 elements in 3 groups (reconstruction).
    const CLens::SSurface s_aoSonnar250mm[] =
    {
        {   137.8564,   8.7060, 1.51680,  24.783, FALSE, 64.17, CLens::GLASS_NONE },
        {   340.6072,   1.0882, 1.00000,  23.548, FALSE,  0.00, CLens::GLASS_NONE },
        {    74.6359,  10.8822, 1.61272,  22.748, FALSE, 58.63, CLens::GLASS_NONE },
        {  -258.1013,   4.3530, 1.62004,  21.189, FALSE, 36.37, CLens::GLASS_NONE },
        {   109.1308,  32.6469, 1.00000,  20.169, FALSE,  0.00, CLens::GLASS_NONE },
        {     0.0000,  38.0881, 1.00000,  15.793, TRUE ,  0.00, CLens::GLASS_NONE },
        {   -46.6768,   3.8087, 1.67270,  15.326, FALSE, 32.21, CLens::GLASS_NONE },
        {   -56.8795,   0.0000, 1.00000,  16.241, FALSE,  0.00, CLens::GLASS_NONE },
    };

        // Zeiss Tele-Tessar 500 mm f/8: a reconstruction, 5 elements in 3 groups (reconstruction).
    const CLens::SSurface s_aoTeleTessar500mm[] =
    {
        {   180.8716,   8.8270, 1.62041,  32.759, FALSE, 60.32, CLens::GLASS_NONE },
        {  -235.3161,   3.5305, 1.62004,  32.288, FALSE, 36.37, CLens::GLASS_NONE },
        {   493.9751,   1.3240, 1.00000,  31.504, FALSE,  0.00, CLens::GLASS_NONE },
        {   106.0275,   7.0615, 1.51680,  30.970, FALSE, 64.17, CLens::GLASS_NONE },
        {   158.2060,  88.2680, 1.00000,  30.156, FALSE,  0.00, CLens::GLASS_NONE },
        {     0.0000,  61.7880, 1.00000,  18.640, TRUE ,  0.00, CLens::GLASS_NONE },
        {   -76.8825,   3.5305, 1.67270,  15.179, FALSE, 32.21, CLens::GLASS_NONE },
        {   140.0795,   6.1790, 1.61272,  15.844, FALSE, 58.63, CLens::GLASS_NONE },
        {  -154.5125,   0.0000, 1.00000,  16.405, FALSE,  0.00, CLens::GLASS_NONE },
    };

    // END APOLLO LENSES

    const CLens::SPrescription s_aoPrescriptions[] =
    {
        { "double-gauss-50mm", "50 mm f/2 double Gauss (US 2,673,491, Tronnier), a classic standard lens",
          s_aoDoubleGauss50, (int)( sizeof( s_aoDoubleGauss50 ) / sizeof( s_aoDoubleGauss50[ 0 ] ) ) },
        // BEGIN APOLLO ENTRIES
        { "biogon-38mm", "Zeiss Biogon 38 mm f/4.5 (US 2,721,499 Ex. 2, Bertele): the SWC's lens",
          s_aoBiogon38mm, (int)( sizeof( s_aoBiogon38mm ) / sizeof( s_aoBiogon38mm[ 0 ] ) ) },
        { "biogon-60mm", "Zeiss Biogon 60 mm f/5.6 (HDC): a reconstruction from the Biogon of US 2,721,499",
          s_aoBiogon60mm, (int)( sizeof( s_aoBiogon60mm ) / sizeof( s_aoBiogon60mm[ 0 ] ) ) },
        { "biogon-60mm-reseau", "Zeiss Biogon 60 mm f/5.6 with the Hasselblad Data Camera's Reseau plate (3 mm, assumed BK7)",
          s_aoBiogon60mmReseau, (int)( sizeof( s_aoBiogon60mmReseau ) / sizeof( s_aoBiogon60mmReseau[ 0 ] ) ) },
        { "planar-80mm", "Zeiss Planar 80 mm f/2.8 (US 2,724,994 Ex. I, Lange): 5 elements, 4 groups",
          s_aoPlanar80mm, (int)( sizeof( s_aoPlanar80mm ) / sizeof( s_aoPlanar80mm[ 0 ] ) ) },
        { "planar-100mm", "Zeiss Planar 100 mm f/3.5: the Planar of US 2,724,994 Ex. II at 100 mm and f/3.5",
          s_aoPlanar100mm, (int)( sizeof( s_aoPlanar100mm ) / sizeof( s_aoPlanar100mm[ 0 ] ) ) },
        { "uv-sonnar-105mm", "Zeiss UV-Sonnar 105 mm f/4.3: a reconstruction, 7 air-spaced silica and CaF2 singlets",
          s_aoUvSonnar105mm, (int)( sizeof( s_aoUvSonnar105mm ) / sizeof( s_aoUvSonnar105mm[ 0 ] ) ) },
        { "sonnar-250mm", "Zeiss Sonnar 250 mm f/5.6: a reconstruction, 4 elements in 3 groups",
          s_aoSonnar250mm, (int)( sizeof( s_aoSonnar250mm ) / sizeof( s_aoSonnar250mm[ 0 ] ) ) },
        { "tele-tessar-500mm", "Zeiss Tele-Tessar 500 mm f/8: a reconstruction, 5 elements in 3 groups",
          s_aoTeleTessar500mm, (int)( sizeof( s_aoTeleTessar500mm ) / sizeof( s_aoTeleTessar500mm[ 0 ] ) ) },
        // END APOLLO ENTRIES
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
                // A Sellmeier glass's d-line index, exactly as traced.
            for ( size_t nSurface = 0; nSurface < m_aoSurfaces.size(); nSurface++ )
            {
                if ( m_aoSurfaces[ nSurface ].nGlass != GLASS_NONE )
                    m_aoSurfaces[ nSurface ].nIor = SellmeierIndex( m_aoSurfaces[ nSurface ].nGlass, WAVELENGTH_D );
            }
            for ( int nChannel = 0; nChannel < 4; nChannel++ )
                m_anIorTable[ nChannel ].clear();
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
    for ( int nChannel = 0; nChannel < 4; nChannel++ )
        m_anIorTable[ nChannel ].clear();
}

FLT CLens::IorAt( int nIndex, int nChannel ) const
{
        // A single wavelength has no table: work it out each time.
    if ( IsSpectral( nChannel ) )
        return WorkOutIor( nIndex, nChannel );

    const std::vector<FLT> &anTable = m_anIorTable[ ( nChannel >= 0 && nChannel < 4 ) ? nChannel : 0 ];

    if ( (size_t)( nIndex + 1 ) < anTable.size() && anTable.size() == m_aoSurfaces.size() + 1 )
        return anTable[ (size_t)( nIndex + 1 ) ];
    return WorkOutIor( nIndex, nChannel );
}

FLT CLens::WorkOutIor( int nIndex, int nChannel ) const
{
    if ( nIndex < 0 )
        return 1.0;

    const SSurface &oSurface = m_aoSurfaces[ (size_t)nIndex ];

    if ( ! m_bChromatic )
        return oSurface.nIor;
    if ( oSurface.nGlass != GLASS_NONE )
    {
            // Green, and every channel traced together, are at the d line,
            // which is the surface's nIor.
        if ( nChannel == CHANNEL_ALL || nChannel == CHANNEL_GREEN )
            return oSurface.nIor;
        if ( IsSpectral( nChannel ) )
            return SellmeierIndex( oSurface.nGlass, ChannelWavelength( nChannel ) / 1000.0 );
        return SellmeierIndex( oSurface.nGlass, ( nChannel == CHANNEL_RED ) ? WAVELENGTH_C : WAVELENGTH_F );
    }
    return DispersedIndex( oSurface.nIor, oSurface.nAbbe, nChannel );
}

FLT CLens::SellmeierIndex( int nGlass, FLT nMicrometers )
{
        // I. H. Malitson, JOSA 55 (1965), fused silica, 0.21-3.71 um, and
        // Applied Optics 2 (1963), calcium fluoride, 0.23-9.7 um.
    static const double s_aadCoefficients[ 2 ][ 6 ] =
    {
        { 0.6961663, 0.4079426, 0.8974794, 0.0684043, 0.1162414, 9.896161 },
        { 0.5675888, 0.4710914, 3.8484723, 0.050263605, 0.1003909, 34.649040 },
    };

    if ( nGlass != GLASS_FUSED_SILICA && nGlass != GLASS_CALCIUM_FLUORIDE )
        return 1.0;

    const double   *adC = s_aadCoefficients[ nGlass - 1 ];
    double          dSquare = (double)nMicrometers * (double)nMicrometers;
    double          dSum = 1.0;

    for ( int nTerm = 0; nTerm < 3; nTerm++ )
        dSum += adC[ nTerm ] * dSquare / ( dSquare - adC[ nTerm + 3 ] * adC[ nTerm + 3 ] );
    return (FLT)sqrt( dSum );
}

const char *CLens::GlassName( int nGlass )
{
    switch ( nGlass )
    {
        case GLASS_FUSED_SILICA:        return "fused-silica";
        case GLASS_CALCIUM_FLUORIDE:    return "calcium-fluoride";
        default:                        return NULL;
    }
}

int CLens::GlassByName( const std::string &strName )
{
    if ( strName == "fused-silica" )
        return GLASS_FUSED_SILICA;
    if ( strName == "calcium-fluoride" )
        return GLASS_CALCIUM_FLUORIDE;
    return GLASS_NONE;
}

int CLens::WavelengthChannel( FLT nNanometers )
{
    return CHANNEL_SPECTRAL + (int)floor( ( nNanometers * 100.0 ) + 0.5 );
}

FLT CLens::IndexAt( FLT nIor, FLT nAbbe, FLT nMicrometers )
{
        // Cauchy's n = A + B / wavelength^2, through n at the d line and the
        // Abbe number V = ( n_d - 1 ) / ( n_F - n_C ).
    FLT nB = ( nIor - 1.0 ) /
             ( nAbbe * ( ( 1.0 / ( WAVELENGTH_F * WAVELENGTH_F ) ) - ( 1.0 / ( WAVELENGTH_C * WAVELENGTH_C ) ) ) );
    FLT nA = nIor - ( nB / ( WAVELENGTH_D * WAVELENGTH_D ) );

    return nA + ( nB / ( nMicrometers * nMicrometers ) );
}

FLT CLens::DispersedIndex( FLT nIor, FLT nAbbe, int nChannel )
{
    if ( nChannel == CHANNEL_ALL || nChannel == CHANNEL_GREEN || ! ( nAbbe > 0.0 ) || nIor == 1.0 )
        return nIor;
    if ( IsSpectral( nChannel ) )
        return IndexAt( nIor, nAbbe, ChannelWavelength( nChannel ) / 1000.0 );
    return IndexAt( nIor, nAbbe, ( nChannel == CHANNEL_RED ) ? WAVELENGTH_C : WAVELENGTH_F );
}

BOOL CLens::IsChromatic( void ) const
{
    if ( ! m_bChromatic )
        return FALSE;
    for ( size_t nIndex = 0; nIndex < m_aoSurfaces.size(); nIndex++ )
    {
        if ( ( m_aoSurfaces[ nIndex ].nAbbe > 0.0 && m_aoSurfaces[ nIndex ].nIor != 1.0 ) ||
             m_aoSurfaces[ nIndex ].nGlass != GLASS_NONE )
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
    for ( int nChannel = 0; nChannel < 4; nChannel++ )
        m_anIorTable[ nChannel ].clear();
    if ( m_aoSurfaces.empty() )
    {
        strError = "a lens needs at least one surface";
        return FALSE;
    }

        // Every index, worked out once.
    for ( int nChannel = 0; nChannel < 4; nChannel++ )
    {
        std::vector<FLT> anTable( m_aoSurfaces.size() + 1 );

        for ( int nIndex = -1; nIndex < (int)m_aoSurfaces.size(); nIndex++ )
            anTable[ (size_t)( nIndex + 1 ) ] = WorkOutIor( nIndex, nChannel );
        m_anIorTable[ nChannel ].swap( anTable );
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
        FLT nRear = m_aoSurfaces[ AimIndex() ].nAperture;
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
                FLT anDirection[ 3 ] = { nX, nY, m_anVertexZ[ AimIndex() ] - m_nSensorZ };

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

        // The exit pupil tables: kept if they are already for this setup,
        // taken from the scene file if it brought ones for it, else worked
        // out.
    m_nPupilReach = ( m_nSensorWidth / 2.0 ) * PUPIL_REACH;

    std::string strKey = TablesKey();

    if ( ! m_pTables || m_strTablesKey != strKey )
    {
        if ( m_pLoadedTables && m_strLoadedKey == strKey )
            m_pTables = m_pLoadedTables;
        else
            BuildTables();
        m_strTablesKey = strKey;
    }
    m_pLoadedTables.reset();
    m_bPrepared = TRUE;
    return TRUE;
}

void CLens::FindPupil( int nRing, SPupil &oPupil ) const
{
    FLT                 nRear = m_aoSurfaces[ AimIndex() ].nAperture;
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
                        FLT anDirection[ 3 ] = { nX - nSensorX, nY, m_anVertexZ[ AimIndex() ] - m_nSensorZ };

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






namespace
{
    void PutBigEndian( std::vector<unsigned char> &anOut, double dValue )
    {
        uint64_t nBits;

        memcpy( &nBits, &dValue, sizeof( nBits ) );
        for ( int nByte = 7; nByte >= 0; nByte-- )
            anOut.push_back( (unsigned char)( nBits >> ( nByte * 8 ) ) );
    }

    double GetBigEndian( const std::vector<unsigned char> &anIn, size_t nAt )
    {
        uint64_t nBits = 0;
        double   dValue;

        for ( int nByte = 0; nByte < 8; nByte++ )
            nBits = ( nBits << 8 ) | anIn[ nAt + (size_t)nByte ];
        memcpy( &dValue, &nBits, sizeof( dValue ) );
        return dValue;
    }

        // The tables as bytes: each ring's center, half widths and whether it
        // is open; then, for each channel, how many shares and the shares.
        // Doubles, big-endian, so they read back exactly anywhere.
    std::vector<unsigned char> TablesToBytes( const std::vector<CLens::SPupil> &aoRings, const std::vector<FLT> *aanPass )
    {
        std::vector<unsigned char> anOut;

        PutBigEndian( anOut, (double)aoRings.size() );
        for ( size_t nRing = 0; nRing < aoRings.size(); nRing++ )
        {
            PutBigEndian( anOut, (double)aoRings[ nRing ].nCenterX );
            PutBigEndian( anOut, (double)aoRings[ nRing ].nCenterY );
            PutBigEndian( anOut, (double)aoRings[ nRing ].nHalfX );
            PutBigEndian( anOut, (double)aoRings[ nRing ].nHalfY );
            PutBigEndian( anOut, aoRings[ nRing ].bOpen ? 1.0 : 0.0 );
        }
        for ( int nChannel = 0; nChannel < 4; nChannel++ )
        {
            PutBigEndian( anOut, (double)aanPass[ nChannel ].size() );
            for ( size_t nKnot = 0; nKnot < aanPass[ nChannel ].size(); nKnot++ )
                PutBigEndian( anOut, (double)aanPass[ nChannel ][ nKnot ] );
        }
        return anOut;
    }

    bool BytesToTables( const std::vector<unsigned char> &anIn, std::vector<CLens::SPupil> &aoRings, std::vector<FLT> *aanPass )
    {
        size_t nAt = 0;

        auto Next = [ & ]( double &dValue ) -> bool
        {
            if ( nAt + 8 > anIn.size() )
                return false;
            dValue = GetBigEndian( anIn, nAt );
            nAt += 8;
            return true;
        };

        double dCount;

        if ( ! Next( dCount ) || dCount != PUPIL_RINGS )
            return false;
        aoRings.assign( PUPIL_RINGS, CLens::SPupil() );
        for ( int nRing = 0; nRing < PUPIL_RINGS; nRing++ )
        {
            double adValue[ 5 ];

            for ( int nValue = 0; nValue < 5; nValue++ )
            {
                if ( ! Next( adValue[ nValue ] ) )
                    return false;
            }
            aoRings[ (size_t)nRing ].nCenterX = (FLT)adValue[ 0 ];
            aoRings[ (size_t)nRing ].nCenterY = (FLT)adValue[ 1 ];
            aoRings[ (size_t)nRing ].nHalfX = (FLT)adValue[ 2 ];
            aoRings[ (size_t)nRing ].nHalfY = (FLT)adValue[ 3 ];
            aoRings[ (size_t)nRing ].bOpen = ( adValue[ 4 ] != 0.0 ) ? TRUE : FALSE;
        }
        for ( int nChannel = 0; nChannel < 4; nChannel++ )
        {
            if ( ! Next( dCount ) || ( dCount != 0.0 && dCount != PUPIL_RINGS + 1 ) )
                return false;
            aanPass[ nChannel ].assign( (size_t)dCount, 0.0 );
            for ( size_t nKnot = 0; nKnot < aanPass[ nChannel ].size(); nKnot++ )
            {
                double dValue;

                if ( ! Next( dValue ) )
                    return false;
                aanPass[ nChannel ][ nKnot ] = (FLT)dValue;
            }
        }
        return nAt == anIn.size();
    }

        // One of the threads working out the tables: takes jobs (a ring, or
        // a channel's share at a ring's edge) until there are none left.
    class CTableWorker : public IASLib::CThread
    {
        protected:
            std::atomic<int>       &m_nNext;
            int                     m_nJobs;
            std::function<void( int )> m_fnJob;

        public:
            CTableWorker( std::atomic<int> &nNext, int nJobs, std::function<void( int )> fnJob ) :
                IASLib::CThread( "lens tables", true ),
                m_nNext( nNext ),
                m_nJobs( nJobs ),
                m_fnJob( fnJob )
            {
                SetNoDelete( true );
                Resume();
            }

            virtual void *Run( void )
            {
                for ( int nJob = m_nNext++; nJob < m_nJobs; nJob = m_nNext++ )
                    m_fnJob( nJob );
                return NULL;
            }
    };
}

std::string CLens::TablesKey( void ) const
{
    std::vector<unsigned char>  anBytes;
    uint64_t                    nHash = 1469598103934665603ull;     // FNV-1a
    char                        achKey[ 32 ];

    PutBigEndian( anBytes, 2.0 );       // the tables' layout and how they are worked out
    PutBigEndian( anBytes, (double)PUPIL_RINGS );
    PutBigEndian( anBytes, (double)PUPIL_GRID );
    PutBigEndian( anBytes, (double)PUPIL_RADII );
    PutBigEndian( anBytes, (double)PUPIL_REACH );
    PutBigEndian( anBytes, (double)EXPOSURE_GRID );
    for ( size_t nIndex = 0; nIndex < m_aoSurfaces.size(); nIndex++ )
    {
        PutBigEndian( anBytes, (double)m_aoSurfaces[ nIndex ].nRadius );
        PutBigEndian( anBytes, (double)m_aoSurfaces[ nIndex ].nThickness );
        PutBigEndian( anBytes, (double)m_aoSurfaces[ nIndex ].nIor );
        PutBigEndian( anBytes, (double)m_aoSurfaces[ nIndex ].nAperture );
        PutBigEndian( anBytes, m_aoSurfaces[ nIndex ].bStop ? 1.0 : 0.0 );
        PutBigEndian( anBytes, (double)m_aoSurfaces[ nIndex ].nAbbe );
        if ( m_aoSurfaces[ nIndex ].nGlass != GLASS_NONE )
            PutBigEndian( anBytes, 100.0 + (double)m_aoSurfaces[ nIndex ].nGlass );
    }
    PutBigEndian( anBytes, (double)m_nStopRadius );
    PutBigEndian( anBytes, (double)m_nSensorZ );
    PutBigEndian( anBytes, (double)m_nSensorWidth );
    PutBigEndian( anBytes, IsChromatic() ? 1.0 : 0.0 );

    for ( size_t nIndex = 0; nIndex < anBytes.size(); nIndex++ )
    {
        nHash ^= anBytes[ nIndex ];
        nHash *= 1099511628211ull;
    }
    snprintf( achKey, sizeof( achKey ), "%016llx", (unsigned long long)nHash );
    return achKey;
}

void CLens::BuildTables( void )
{
    std::shared_ptr<STables>    pTables = std::make_shared<STables>();
    std::vector<int>            anChannels;
    std::atomic<int>            nNext( 0 );

        // The channels rays are traced in: red, green and blue apart for a
        // chromatic lens, else all at once.
    if ( IsChromatic() )
        anChannels = { CHANNEL_RED, CHANNEL_GREEN, CHANNEL_BLUE };
    else
        anChannels = { CHANNEL_ALL };

    pTables->aoRings.assign( PUPIL_RINGS, SPupil() );
    for ( size_t nIndex = 0; nIndex < anChannels.size(); nIndex++ )
        pTables->anPass[ anChannels[ nIndex ] ].assign( PUPIL_RINGS + 1, 0.0 );

        // Jobs: every ring, then every channel's share at every ring edge.
    int  nJobs = PUPIL_RINGS + ( (int)anChannels.size() * ( PUPIL_RINGS + 1 ) );
    auto Job = [ & ]( int nJob )
    {
        if ( nJob < PUPIL_RINGS )
            FindPupil( nJob, pTables->aoRings[ (size_t)nJob ] );
        else
        {
            int nShare = nJob - PUPIL_RINGS;
            int nChannel = anChannels[ (size_t)( nShare / ( PUPIL_RINGS + 1 ) ) ];
            int nKnot = nShare % ( PUPIL_RINGS + 1 );

            pTables->anPass[ nChannel ][ (size_t)nKnot ] = FindPass( nKnot, nChannel );
        }
    };

    int nThreads = (int)std::thread::hardware_concurrency();

    nThreads = ( nThreads < 1 ) ? 1 : ( nThreads > nJobs ) ? nJobs : nThreads;

    std::vector<CTableWorker *> apWorkers;

    for ( int nThread = 0; nThread < nThreads; nThread++ )
        apWorkers.push_back( new CTableWorker( nNext, nJobs, Job ) );
    for ( size_t nThread = 0; nThread < apWorkers.size(); nThread++ )
    {
        apWorkers[ nThread ]->Join();
        delete apWorkers[ nThread ];
    }
    m_pTables = pTables;
}

FLT CLens::FindPass( int nKnot, int nChannel ) const
{
    FLT nRear = m_aoSurfaces[ AimIndex() ].nAperture;
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
            FLT anDirection[ 3 ] = { nX - nSensorX, nY, m_anVertexZ[ AimIndex() ] - m_nSensorZ };

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
    FLT nRear = m_aoSurfaces[ AimIndex() ].nAperture;
    FLT nX = 0.0;
    FLT nY = 0.0;

        // The image lands upside down and mirrored on the sensor.
    anOrigin[ 0 ] = -nScreenX * nHalfWidth;
    anOrigin[ 1 ] = -nScreenY * nHalfHeight;
    anOrigin[ 2 ] = m_nSensorZ;

    FLT nFromCenter = SQRT( ( anOrigin[ 0 ] * anOrigin[ 0 ] ) + ( anOrigin[ 1 ] * anOrigin[ 1 ] ) );
    int nRing = ( m_nPupilReach > 0.0 ) ? (int)( ( nFromCenter / m_nPupilReach ) * PUPIL_RINGS ) : PUPIL_RINGS;

    if ( nRing < 0 || nRing >= PUPIL_RINGS || ! m_pTables )
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
        const SPupil &oPupil = m_pTables->aoRings[ (size_t)nRing ];

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
            anDirection[ 2 ] = m_anVertexZ[ AimIndex() ] - anOrigin[ 2 ];
            Normalize( anDirection );
            if ( Trace( anOrigin, anDirection, FALSE, m_nStopRadius, nChannel ) )
            {
                if ( pnWeight )
                {
                        // The share that gets through, between this ring's
                        // edges.
                    FLT                     nAcross = ( ( nFromCenter / m_nPupilReach ) * PUPIL_RINGS ) - nRing;
                    int                     nSlot = ( nChannel >= CHANNEL_RED && nChannel <= CHANNEL_BLUE ) ? nChannel : CHANNEL_ALL;

                        // A single wavelength takes the share of the
                        // nearest of the three channels the lens measured.
                    if ( IsSpectral( nChannel ) )
                    {
                        FLT nMicrometers = ChannelWavelength( nChannel ) / 1000.0;

                        nSlot = ( nMicrometers >= ( WAVELENGTH_C + WAVELENGTH_D ) / 2.0 ) ? CHANNEL_RED :
                                ( nMicrometers >= ( WAVELENGTH_D + WAVELENGTH_F ) / 2.0 ) ? CHANNEL_GREEN : CHANNEL_BLUE;
                    }
                    const std::vector<FLT> &anPass = m_pTables->anPass[ nSlot ];

                        // A channel that isn't traced (a chromatic lens's
                        // all-channel ray) has no share: it is only aimed.
                    *pnWeight = anPass.empty() ? 1.0 :
                                ( anPass[ (size_t)nRing ] * ( 1.0 - nAcross ) ) + ( anPass[ (size_t)nRing + 1 ] * nAcross );
                }
                return TRUE;
            }
        }
        return FALSE;       // nothing got through: black
    }

    anDirection[ 0 ] = nX - anOrigin[ 0 ];
    anDirection[ 1 ] = nY - anOrigin[ 1 ];
    anDirection[ 2 ] = m_anVertexZ[ AimIndex() ] - anOrigin[ 2 ];
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
            if ( oSurface.nGlass == GLASS_NONE )
                RayJson::PutNumber( *pSurface, "ior", oSurface.nIor );
            RayJson::PutNumber( *pSurface, "aperture", oSurface.nAperture );
            if ( oSurface.bStop )
                RayJson::PutBool( *pSurface, "stop", TRUE );
            if ( oSurface.nGlass != GLASS_NONE )
                pSurface->put( "glass", GlassName( oSurface.nGlass ) );
            else if ( oSurface.nAbbe > 0.0 )
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
    if ( m_pTables )
    {
            // The exit pupil tables, exactly, with the fingerprint of the
            // setup they are for, so tasks and later loads needn't work them
            // out again.
        IASLib::CObjectNode *pTables = oNode.putObject( "exitPupil" );

        pTables->put( "key", m_strTablesKey );
        pTables->putBinary( "data", TablesToBytes( m_pTables->aoRings, m_pTables->anPass ) );
    }
}

BOOL CLens::FromJson( const IASLib::CJsonNode &oNode, BOOL bHasLens )
{
    static const char *const astrFields[] = { "prescription", "surfaces", "fNumber", "focusDistance", "sensorWidth",
                                              "millimetersPerUnit", "seed", "chromatic", "exitPupil", NULL };
    static const char *const astrSurface[] = { "radius", "thickness", "ior", "aperture", "stop", "abbe", "glass", NULL };
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

    const IASLib::CJsonNode *pTables = oNode.get( "exitPupil" );

    m_pLoadedTables.reset();
    m_strLoadedKey.clear();
    if ( pTables )
    {
        static const char *const astrTables[] = { "key", "data", NULL };
        RayJson::CWhere          oWhere( "exitPupil" );
        std::string              strKey;
        const IASLib::CJsonNode *pData = pTables->isObject() ? pTables->get( "data" ) : NULL;
        std::vector<unsigned char> anData;
        std::shared_ptr<STables> pLoaded = std::make_shared<STables>();

        if ( ! RayJson::OnlyFields( *pTables, astrTables ) || ! RayJson::GetString( *pTables, "key", strKey, TRUE ) )
            return FALSE;
        if ( pData == NULL || ! pData->binaryValue( anData ) || ! BytesToTables( anData, pLoaded->aoRings, pLoaded->anPass ) )
            return RayJson::Fail( "data must be the exit pupil tables, in base64" );
        m_pLoadedTables = pLoaded;
        m_strLoadedKey = strKey;
    }

    if ( m_nFNumber < 0.0 )
        return RayJson::Fail( "fNumber must be positive" );
    if ( m_nFocusDistance < 0.0 )
        return RayJson::Fail( "focusDistance must be positive" );
    if ( ! ( m_nSensorWidth > 0.0 ) )
        return RayJson::Fail( "sensorWidth must be positive" );
    if ( ! ( m_nMillimetersPerUnit > 0.0 ) )
        return RayJson::Fail( "millimetersPerUnit must be positive" );

    const IASLib::CJsonNode *pSurfaces = oNode.get( "surfaces" );

    if ( bHasLens && pSurfaces == NULL && strPrescription.empty() && ! m_aoSurfaces.empty() )
        return TRUE;
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
        SSurface                    oSurface = { 0.0, 0.0, 1.0, 0.0, FALSE, 0.0, GLASS_NONE };
        std::string                 strGlass;

        if ( ! RayJson::OnlyFields( oSurfaceNode, astrSurface ) ||
             ! RayJson::GetNumber( oSurfaceNode, "radius", oSurface.nRadius, TRUE ) ||
             ! RayJson::GetNumber( oSurfaceNode, "thickness", oSurface.nThickness, TRUE ) ||
             ! RayJson::GetNumber( oSurfaceNode, "ior", oSurface.nIor, FALSE ) ||
             ! RayJson::GetNumber( oSurfaceNode, "aperture", oSurface.nAperture, TRUE ) ||
             ! RayJson::GetBool( oSurfaceNode, "stop", oSurface.bStop, FALSE ) ||
             ! RayJson::GetNumber( oSurfaceNode, "abbe", oSurface.nAbbe, FALSE ) ||
             ! RayJson::GetString( oSurfaceNode, "glass", strGlass, FALSE ) )
            return FALSE;
        if ( oSurfaceNode.get( "glass" ) != NULL )
        {
            RayJson::CWhere oGlass( "glass" );

            if ( oSurfaceNode.get( "ior" ) != NULL || oSurfaceNode.get( "abbe" ) != NULL )
                return RayJson::Fail( "a surface gives a glass by name or its ior and abbe, not both" );
            oSurface.nGlass = GlassByName( strGlass );
            if ( oSurface.nGlass == GLASS_NONE )
                return RayJson::Fail( "unknown glass \"" + strGlass + "\" (fused-silica or calcium-fluoride)" );
            oSurface.nIor = SellmeierIndex( oSurface.nGlass, WAVELENGTH_D );
        }
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
