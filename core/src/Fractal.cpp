/*************************************************************************
*   CFractal                                                             *
*                                                                        *
*   A fractal, by sphere tracing a distance estimate. See Fractal.h.     *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Fractal.h"
#include "SceneJson.h"

#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CFractal, "fractal" );

namespace
{
    const char *const s_astrKinds[] = { "mandelbulb", "menger", "sierpinski" };

    inline double Length( const double v[ 3 ] )
    {
        return sqrt( ( v[ 0 ] * v[ 0 ] ) + ( v[ 1 ] * v[ 1 ] ) + ( v[ 2 ] * v[ 2 ] ) );
    }

    inline double Larger( double a, double b ) { return ( a > b ) ? a : b; }
    inline double Smaller( double a, double b ) { return ( a < b ) ? a : b; }

        // GLSL's mod: x - y floor( x / y ), never negative for y > 0.
    inline double Modulo( double x, double y ) { return x - ( y * floor( x / y ) ); }

        // The distance to a box of half-size 1 (negative inside).
    double UnitBox( const double p[ 3 ] )
    {
        double q[ 3 ] = { fabs( p[ 0 ] ) - 1.0, fabs( p[ 1 ] ) - 1.0, fabs( p[ 2 ] ) - 1.0 };
        double o[ 3 ] = { Larger( q[ 0 ], 0.0 ), Larger( q[ 1 ], 0.0 ), Larger( q[ 2 ], 0.0 ) };

        return Length( o ) + Smaller( Larger( q[ 0 ], Larger( q[ 1 ], q[ 2 ] ) ), 0.0 );
    }

    double Mandelbulb( const double p[ 3 ], int nIterations, double nPower, double nBailout )
    {
        double z[ 3 ] = { p[ 0 ], p[ 1 ], p[ 2 ] };
        double dr = 1.0, r = Length( z );

        for ( int i = 0; i < nIterations; i++ )
        {
            r = Length( z );
            if ( r > nBailout || r < 1e-300 )
                break;

            double nCos = z[ 1 ] / r;

            nCos = ( nCos > 1.0 ) ? 1.0 : ( nCos < -1.0 ) ? -1.0 : nCos;

            double nTheta = acos( nCos ) * nPower;
            double nPhi = atan2( z[ 2 ], z[ 0 ] ) * nPower;
            double nRadius = pow( r, nPower );

            dr = ( pow( r, nPower - 1.0 ) * nPower * dr ) + 1.0;
            z[ 0 ] = ( nRadius * sin( nTheta ) * cos( nPhi ) ) + p[ 0 ];
            z[ 1 ] = ( nRadius * cos( nTheta ) ) + p[ 1 ];
            z[ 2 ] = ( nRadius * sin( nTheta ) * sin( nPhi ) ) + p[ 2 ];
        }
        if ( r < 1e-300 )
            return -1e-3;
        return 0.5 * log( r ) * r / dr;
    }

        // Inigo Quilez's Menger sponge estimate: the cube, then crosses cut
        // out at ever finer scales.
    double Menger( const double p[ 3 ], int nIterations )
    {
        double d = UnitBox( p );
        double s = 1.0;

        for ( int m = 0; m < nIterations; m++ )
        {
            double a[ 3 ], r[ 3 ];

            for ( int i = 0; i < 3; i++ )
                a[ i ] = Modulo( p[ i ] * s, 2.0 ) - 1.0;
            s *= 3.0;
            for ( int i = 0; i < 3; i++ )
                r[ i ] = fabs( 1.0 - ( 3.0 * fabs( a[ i ] ) ) );

            double da = Larger( r[ 0 ], r[ 1 ] ), db = Larger( r[ 1 ], r[ 2 ] ), dc = Larger( r[ 2 ], r[ 0 ] );
            double c = ( Smaller( da, Smaller( db, dc ) ) - 1.0 ) / s;

            d = Larger( d, c );
        }
        return d;
    }

        // Folds a point toward the corner (1, 1, 1) and doubles it, each
        // time; then the distance to the tetrahedron, scaled back.
    double Sierpinski( const double p[ 3 ], int nIterations )
    {
        double z[ 3 ] = { p[ 0 ], p[ 1 ], p[ 2 ] };
        double nScale = 1.0;

        for ( int n = 0; n < nIterations; n++ )
        {
            double t;

            if ( z[ 0 ] + z[ 1 ] < 0.0 )
            {
                t = -z[ 1 ];
                z[ 1 ] = -z[ 0 ];
                z[ 0 ] = t;
            }
            if ( z[ 0 ] + z[ 2 ] < 0.0 )
            {
                t = -z[ 2 ];
                z[ 2 ] = -z[ 0 ];
                z[ 0 ] = t;
            }
            if ( z[ 1 ] + z[ 2 ] < 0.0 )
            {
                t = -z[ 2 ];
                z[ 2 ] = -z[ 1 ];
                z[ 1 ] = t;
            }
            for ( int i = 0; i < 3; i++ )
                z[ i ] = ( 2.0 * z[ i ] ) - 1.0;
            nScale *= 0.5;
        }

            // The tetrahedron with corners at (1, 1, 1), (-1, -1, 1),
            // (-1, 1, -1) and (1, -1, -1): four planes.
        double nPlanes = Larger( Larger( -z[ 0 ] - z[ 1 ] - z[ 2 ], z[ 0 ] + z[ 1 ] - z[ 2 ] ),
                                 Larger( -z[ 0 ] + z[ 1 ] + z[ 2 ], z[ 0 ] - z[ 1 ] + z[ 2 ] ) );

        return ( nPlanes - 1.0 ) / sqrt( 3.0 ) * nScale;
    }
}

CFractal::CFractal( void ) :
    m_nKind( KIND_MENGER ),
    m_nIterations( 3 ),
    m_nPower( 8.0 ),
    m_nBailout( 2.0 ),
    m_nEpsilon( 1e-4 ),
    m_nMaxSteps( 256 ),
    m_nStepFactor( 1.0 )
{
    Prepare();
}

CFractal::~CFractal( void )
{
}

int CFractal::DefaultIterations( int nKind )
{
    return ( nKind == KIND_MANDELBULB ) ? 10 : ( nKind == KIND_MENGER ) ? 3 : 6;
}

BOOL CFractal::Set( const CVector &oCenter, int nKind, int nIterations, FLT nScale )
{
    m_oCenter = oCenter;
    m_nKind = nKind;
    m_nIterations = nIterations;
    m_nScale = nScale;
    return Prepare();
}

BOOL CFractal::PrepareField( void )
{
    if ( m_nKind < KIND_MANDELBULB || m_nKind > KIND_SIERPINSKI )
        return RayJson::Fail( "kind must be mandelbulb, menger or sierpinski" );
    if ( m_nIterations < 0 || m_nIterations > 64 )
        return RayJson::Fail( "iterations must be from 0 to 64" );
    if ( ! ( m_nPower >= 2.0 && m_nPower <= 32.0 ) )
        return RayJson::Fail( "power must be from 2 to 32" );
    if ( ! ( m_nBailout > 1.0 ) )
        return RayJson::Fail( "bailout must be more than 1" );
    if ( ! ( m_nEpsilon > 0.0 && m_nEpsilon < 0.1 ) )
        return RayJson::Fail( "epsilon must be more than 0 and less than 0.1" );
    if ( m_nMaxSteps < 1 || m_nMaxSteps > 100000 )
        return RayJson::Fail( "maxSteps must be from 1 to 100000" );
    if ( ! ( m_nStepFactor > 0.0 && m_nStepFactor <= 1.0 ) )
        return RayJson::Fail( "stepFactor must be more than 0 and at most 1" );
    return TRUE;
}

double CFractal::Estimate( const double anPoint[ 3 ] ) const
{
    switch ( m_nKind )
    {
        case KIND_MANDELBULB:
            return Mandelbulb( anPoint, m_nIterations, m_nPower, m_nBailout );
        case KIND_MENGER:
            return Menger( anPoint, m_nIterations );
        default:
            return Sierpinski( anPoint, m_nIterations );
    }
}

double CFractal::Field( const double anPoint[ 3 ] ) const
{
        // The surface the tracer stops at is where the estimate is epsilon.
    return Estimate( anPoint ) - m_nEpsilon;
}

void CFractal::FieldBounds( double anLow[ 3 ], double anHigh[ 3 ] ) const
{
    double nHalf = 1.0;

    if ( m_nKind == KIND_MANDELBULB )
        nHalf = ( m_nPower >= 6.0 ) ? 1.25 : ( m_nPower >= 3.0 ) ? 1.6 : 2.1;
    nHalf += 4.0 * m_nEpsilon;
    for ( int i = 0; i < 3; i++ )
    {
        anLow[ i ] = -nHalf;
        anHigh[ i ] = nHalf;
    }
}

BOOL CFractal::FieldHit( const double anOrigin[ 3 ], const double anDirection[ 3 ], double nLow, double nHigh, double &nT ) const
{
    double anLow[ 3 ], anHigh[ 3 ];
    double nFrom = nLow, nTo = nHigh;

    FieldBounds( anLow, anHigh );
    if ( ! ClipToBox( anOrigin, anDirection, anLow, anHigh, nFrom, nTo ) )
        return FALSE;

    double t = nFrom;
    double p[ 3 ];

        // A ray that starts on the surface (leaving it) steps off it first.
    if ( nFrom <= nLow )
    {
        for ( int nPush = 0; nPush < 16; nPush++ )
        {
            for ( int i = 0; i < 3; i++ )
                p[ i ] = anOrigin[ i ] + ( t * anDirection[ i ] );
            if ( Estimate( p ) >= m_nEpsilon )
                break;
            t += 2.0 * m_nEpsilon;
        }
    }
    for ( int nStep = 0; nStep < m_nMaxSteps && t <= nTo; nStep++ )
    {
        for ( int i = 0; i < 3; i++ )
            p[ i ] = anOrigin[ i ] + ( t * anDirection[ i ] );

        double d = Estimate( p );

        if ( d < m_nEpsilon )
        {
            if ( t > nLow && t < nHigh )
            {
                nT = t;
                return TRUE;
            }
            return FALSE;
        }
        t += d * m_nStepFactor;
    }
    return FALSE;
}

void CFractal::FieldNormal( const double anPoint[ 3 ], double anNormal[ 3 ] ) const
{
    FieldGradient( anPoint, ( m_nEpsilon > 1e-7 ) ? m_nEpsilon : 1e-7, anNormal );
}

const char *const *CFractal::ShapeFields( void )
{
    static const char *const astrFields[] = { "kind", "iterations", "power", "bailout", "epsilon", "maxSteps", "stepFactor", "scale",
                                              NULL };

    return astrFields;
}

void CFractal::FieldToJson( IASLib::CObjectNode &oNode )
{
    oNode.put( "kind", s_astrKinds[ m_nKind ] );
    if ( m_nIterations != DefaultIterations( m_nKind ) )
        RayJson::PutNumber( oNode, "iterations", m_nIterations );
    if ( m_nKind == KIND_MANDELBULB )
    {
        if ( m_nPower != 8.0 )
            RayJson::PutNumber( oNode, "power", m_nPower );
        if ( m_nBailout != 2.0 )
            RayJson::PutNumber( oNode, "bailout", m_nBailout );
    }
    if ( m_nEpsilon != 1e-4 )
        RayJson::PutNumber( oNode, "epsilon", m_nEpsilon );
    if ( m_nMaxSteps != 256 )
        RayJson::PutNumber( oNode, "maxSteps", m_nMaxSteps );
    if ( m_nStepFactor != 1.0 )
        RayJson::PutNumber( oNode, "stepFactor", m_nStepFactor );
}

BOOL CFractal::FieldFromJson( const IASLib::CJsonNode &oNode )
{
    std::string strKind;

    if ( ! RayJson::GetString( oNode, "kind", strKind, TRUE ) )
        return FALSE;
    m_nKind = -1;
    for ( int n = 0; n < 3; n++ )
    {
        if ( strKind == s_astrKinds[ n ] )
            m_nKind = n;
    }
    if ( m_nKind < 0 )
    {
        RayJson::CWhere oWhere( "kind" );

        return RayJson::Fail( "kind must be mandelbulb, menger or sierpinski, not \"" + strKind + "\"" );
    }
    m_nIterations = DefaultIterations( m_nKind );
    m_nPower = 8.0;
    m_nBailout = 2.0;
    m_nEpsilon = 1e-4;
    m_nMaxSteps = 256;
    m_nStepFactor = 1.0;
    if ( m_nKind != KIND_MANDELBULB && ( oNode.get( "power" ) || oNode.get( "bailout" ) ) )
    {
        RayJson::CWhere oWhere( oNode.get( "power" ) ? "power" : "bailout" );

        return RayJson::Fail( "only a mandelbulb has a power and a bailout" );
    }
    return RayJson::GetInt( oNode, "iterations", m_nIterations, 0, 64, FALSE ) &&
           RayJson::GetNumber( oNode, "power", m_nPower, FALSE ) &&
           RayJson::GetNumber( oNode, "bailout", m_nBailout, FALSE ) &&
           RayJson::GetNumber( oNode, "epsilon", m_nEpsilon, FALSE ) &&
           RayJson::GetInt( oNode, "maxSteps", m_nMaxSteps, 1, 100000, FALSE ) &&
           RayJson::GetNumber( oNode, "stepFactor", m_nStepFactor, FALSE );
}

void CFractal::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CFractal::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
