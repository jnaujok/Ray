/*************************************************************************
*   CLathe                                                               *
*                                                                        *
*   A solid of revolution. See Lathe.h.                                  *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Lathe.h"
#include "SceneJson.h"

#include <cmath>


DYNAMIC_IMPLEMENTS( CLathe, "lathe" );

namespace
{
    const double s_nTwoPi = 2.0 * PI;
    const double s_nReach = 1e-9;       // how far past its ends a piece's parameter is searched

    double Poly( const double *anC, int nDegree, double s )
    {
        double v = 0.0;

        for ( int i = nDegree; i >= 0; i-- )
            v = ( v * s ) + anC[ i ];
        return v;
    }

    double Slope( const double *anC, int nDegree, double s )
    {
        double v = 0.0;

        for ( int i = nDegree; i >= 1; i-- )
            v = ( v * s ) + ( i * anC[ i ] );
        return v;
    }

        // The product of two polynomials.
    int Multiply( const double *anA, int nA, const double *anB, int nB, double *anOut )
    {
        for ( int i = 0; i <= nA + nB; i++ )
            anOut[ i ] = 0.0;
        for ( int i = 0; i <= nA; i++ )
        {
            for ( int j = 0; j <= nB; j++ )
                anOut[ i + j ] += anA[ i ] * anB[ j ];
        }
        return nA + nB;
    }

        // The roots in (nLow, nHigh) of a polynomial whose top terms may be
        // zero (or nearly, beside its other terms).
    int Roots( double *anC, int nDegree, double nLow, double nHigh, double *anRoots )
    {
        double nMost = 0.0;

        for ( int i = 0; i <= nDegree; i++ )
            nMost = ( std::fabs( anC[ i ] ) > nMost ) ? std::fabs( anC[ i ] ) : nMost;
        if ( nMost == 0.0 )
            return 0;
        while ( nDegree > 0 && std::fabs( anC[ nDegree ] ) <= 1e-13 * nMost )
            nDegree--;
        if ( nDegree == 0 )
            return 0;
        return CLocalShape::PolynomialRoots( anC, nDegree, nLow, nHigh, anRoots );
    }
}

CLathe::CLathe( void ) :
    m_bSmooth( FALSE ),
    m_bCapped( FALSE ),
    m_nSweep( 2.0 * PI ),
    m_bClosed( FALSE ),
    m_bPartial( FALSE ),
    m_nOrient( 1.0 ),
    m_nRMax( 1.0 ),
    m_nYMin( 0.0 ),
    m_nYMax( 1.0 ),
    m_bAnyHit( FALSE ),
    m_nNearR( 0.0 ),
    m_nNearY( 0.0 ),
    m_nNearPiece( 0 ),
    m_nNearS( 0.0 ),
    m_nNearDistance( 0.0 ),
    m_bNearValid( FALSE )
{
        // A plain cylinder until a scene says otherwise.
    Profile::SEntry aoEntries[ 4 ] = { { 1, { { 0.0, 0.0 } } }, { 1, { { 1.0, 0.0 } } }, { 1, { { 1.0, 1.0 } } }, { 1, { { 0.0, 1.0 } } } };

    m_oProfile.aoEntries.assign( aoEntries, aoEntries + 4 );
    Profile::MakePieces( m_oProfile );
    Prepare();
}

CLathe::~CLathe( void )
{
}

BOOL CLathe::PrepareShape( void )
{
    if ( m_oProfile.aoPieces.empty() )
        return RayJson::Fail( "profile needs at least two points" );
    if ( ! ( m_nSweep > 0.0 ) || m_nSweep > s_nTwoPi + 1e-12 )
        return RayJson::Fail( "sweep must be more than 0 and at most 360 degrees" );

    double nRMax = 0.0;

    for ( const Profile::SPiece &oPiece : m_oProfile.aoPieces )
    {
        for ( int k = 0; k < 4; k++ )
        {
            if ( oPiece.p[ k ][ 0 ] < 0.0 )
                return RayJson::Fail( "profile's r (the first number of each point) can't be below 0" );
            nRMax = ( oPiece.p[ k ][ 0 ] > nRMax ) ? oPiece.p[ k ][ 0 ] : nRMax;
        }
    }
    if ( ! ( nRMax > 0.0 ) )
        return RayJson::Fail( "profile lies on the axis, so it has no surface" );

    const Profile::SPiece &oFirst = m_oProfile.aoPieces.front(), &oLast = m_oProfile.aoPieces.back();
    double                 anStart[ 2 ] = { oFirst.p[ 0 ][ 0 ], oFirst.p[ 0 ][ 1 ] };
    double                 anEnd[ 2 ] = { oLast.p[ 3 ][ 0 ], oLast.p[ 3 ][ 1 ] };
    double                 nOnAxis = 1e-12 * nRMax;
    BOOL                   bStartOnAxis = anStart[ 0 ] <= nOnAxis, bEndOnAxis = anEnd[ 0 ] <= nOnAxis;
    std::vector<Profile::SPiece> aoLoop = m_oProfile.aoPieces;

        // Flat discs closing the open ends, in to the axis: running from
        // the end to the axis, and from the axis out to the start, so the
        // loop goes the same way round.
    auto AddLine = [&]( double r0, double y0, double r1, double y1 )
    {
        Profile::SPiece oPiece;

        oPiece.nDegree = 1;
        for ( int k = 0; k < 4; k++ )
        {
            oPiece.p[ k ][ 0 ] = r0 + ( ( r1 - r0 ) * k / 3.0 );
            oPiece.p[ k ][ 1 ] = y0 + ( ( y1 - y0 ) * k / 3.0 );
        }
        oPiece.p[ 3 ][ 0 ] = r1;
        oPiece.p[ 3 ][ 1 ] = y1;
        aoLoop.push_back( oPiece );
    };

    if ( m_bCapped && ! bEndOnAxis )
        AddLine( anEnd[ 0 ], anEnd[ 1 ], 0.0, anEnd[ 1 ] );
    if ( m_bCapped && ! bStartOnAxis )
        AddLine( 0.0, anStart[ 1 ], anStart[ 0 ], anStart[ 1 ] );
    m_bClosed = ( bStartOnAxis || m_bCapped ) && ( bEndOnAxis || m_bCapped );
    m_bPartial = m_nSweep < s_nTwoPi - 1e-12;

        // The area the profile closes against the axis, for the end faces
        // and for which way the loop runs.
    m_oRegion = Profile::Flatten( m_oProfile, 1e-5 * Profile::Extent( m_oProfile ) );
    if ( ! bEndOnAxis )
    {
        Outline::SPoint p = { 0.0, anEnd[ 1 ] };

        m_oRegion.push_back( p );
    }
    if ( ! bStartOnAxis )
    {
        Outline::SPoint p = { 0.0, anStart[ 1 ] };

        m_oRegion.push_back( p );
    }
    m_nOrient = ( m_bClosed && Outline::SignedArea2( m_oRegion ) < 0.0 ) ? -1.0 : 1.0;

        // Each piece as polynomials, its bounds, and v by length along.
    std::vector<double> anLength;
    double              nTotal = 0.0;

    m_aoPieces.clear();
    m_bNearValid = FALSE;
    m_nYMin = 1e300;
    m_nYMax = -1e300;
    for ( const Profile::SPiece &oIn : aoLoop )
    {
        SPiece oPiece;
        double anPrevious[ 2 ], nLength = 0.0;

        oPiece.nDegree = oIn.nDegree;
        Profile::PowerBasis( oIn, oPiece.anR, oPiece.anY );
        oPiece.nRMin = 1e300;
        oPiece.nRMax = 0.0;
        oPiece.nYMin = 1e300;
        oPiece.nYMax = -1e300;
        for ( int k = 0; k < 4; k++ )
        {
            oPiece.nRMin = ( oIn.p[ k ][ 0 ] < oPiece.nRMin ) ? oIn.p[ k ][ 0 ] : oPiece.nRMin;
            oPiece.nRMax = ( oIn.p[ k ][ 0 ] > oPiece.nRMax ) ? oIn.p[ k ][ 0 ] : oPiece.nRMax;
            oPiece.nYMin = ( oIn.p[ k ][ 1 ] < oPiece.nYMin ) ? oIn.p[ k ][ 1 ] : oPiece.nYMin;
            oPiece.nYMax = ( oIn.p[ k ][ 1 ] > oPiece.nYMax ) ? oIn.p[ k ][ 1 ] : oPiece.nYMax;
        }
        m_nYMin = ( oPiece.nYMin < m_nYMin ) ? oPiece.nYMin : m_nYMin;
        m_nYMax = ( oPiece.nYMax > m_nYMax ) ? oPiece.nYMax : m_nYMax;
        Profile::Evaluate( oIn, 0.0, anPrevious );
        for ( int k = 1; k <= 16; k++ )
        {
            double p[ 2 ];

            Profile::Evaluate( oIn, k / 16.0, p );
            nLength += std::hypot( p[ 0 ] - anPrevious[ 0 ], p[ 1 ] - anPrevious[ 1 ] );
            anPrevious[ 0 ] = p[ 0 ];
            anPrevious[ 1 ] = p[ 1 ];
        }
        oPiece.nV0 = nTotal;
        nTotal += nLength;
        oPiece.nV1 = nTotal;
        m_aoPieces.push_back( oPiece );
    }
    for ( SPiece &oPiece : m_aoPieces )
    {
        oPiece.nV0 = ( nTotal > 0.0 ) ? oPiece.nV0 / nTotal : 0.0;
        oPiece.nV1 = ( nTotal > 0.0 ) ? oPiece.nV1 / nTotal : 1.0;
    }
    m_nRMax = nRMax;

        // The end faces of a partial sweep: at angle 0 (the half plane
        // z = 0, x > 0) facing -z, and at the sweep facing on round.
    m_anEndAlong[ 0 ][ 0 ] = 1.0;
    m_anEndAlong[ 0 ][ 1 ] = 0.0;
    m_anEndAlong[ 0 ][ 2 ] = 0.0;
    m_anEndNormal[ 0 ][ 0 ] = 0.0;
    m_anEndNormal[ 0 ][ 1 ] = 0.0;
    m_anEndNormal[ 0 ][ 2 ] = -1.0;
    m_anEndAlong[ 1 ][ 0 ] = std::cos( m_nSweep );
    m_anEndAlong[ 1 ][ 1 ] = 0.0;
    m_anEndAlong[ 1 ][ 2 ] = std::sin( m_nSweep );
    m_anEndNormal[ 1 ][ 0 ] = -std::sin( m_nSweep );
    m_anEndNormal[ 1 ][ 1 ] = 0.0;
    m_anEndNormal[ 1 ][ 2 ] = std::cos( m_nSweep );
    return TRUE;
}

BOOL CLathe::InSweep( double x, double z ) const
{
    if ( ! m_bPartial )
        return TRUE;

    double nAngle = std::atan2( z, x );

    if ( nAngle < 0.0 )
        nAngle += s_nTwoPi;
    return nAngle <= m_nSweep + 1e-9 || nAngle >= s_nTwoPi - 1e-9;
}

BOOL CLathe::InRegion( double r, double y ) const
{
    BOOL   bInside = FALSE;
    size_t nN = m_oRegion.size();

    for ( size_t i = 0, j = nN - 1; i < nN; j = i++ )
    {
        const Outline::SPoint &a = m_oRegion[ i ], &b = m_oRegion[ j ];

        if ( ( a.y > y ) != ( b.y > y ) && r < a.x + ( ( b.x - a.x ) * ( y - a.y ) / ( b.y - a.y ) ) )
            bInside = ! bInside;
    }
    return bInside;
}

BOOL CLathe::PieceHit( const SPiece &oPiece, const FLT anO[ 3 ], const FLT anD[ 3 ], double nLow, double nHigh, double &nBest )
{
    double ox = anO[ 0 ], oy = anO[ 1 ], oz = anO[ 2 ], dx = anD[ 0 ], dy = anD[ 1 ], dz = anD[ 2 ];
    double A = ( dx * dx ) + ( dz * dz ), B = 2.0 * ( ( ox * dx ) + ( oz * dz ) ), C = ( ox * ox ) + ( oz * oz );
    double nMargin = 1e-9 * ( m_nRMax + ( m_nYMax - m_nYMin ) );
    double ta = nLow, tb = nHigh;

        // Only where the ray is within the piece's height and reach.
    if ( std::fabs( dy ) > 1e-12 )
    {
        double t1 = ( oPiece.nYMin - nMargin - oy ) / dy, t2 = ( oPiece.nYMax + nMargin - oy ) / dy;

        ta = ( ( t1 < t2 ) ? t1 : t2 );
        tb = ( ( t1 < t2 ) ? t2 : t1 );
        ta = ( ta > nLow ) ? ta : nLow;
        tb = ( tb < nHigh ) ? tb : nHigh;
        if ( ta > tb )
            return FALSE;
    }
    else if ( oy < oPiece.nYMin - nMargin || oy > oPiece.nYMax + nMargin )
        return FALSE;

    double tm = ta;

    if ( A > 0.0 )
    {
        tm = -B / ( 2.0 * A );
        tm = ( tm < ta ) ? ta : ( tm > tb ) ? tb : tm;
    }
    if ( ( A * tm * tm ) + ( B * tm ) + C > ( oPiece.nRMax + nMargin ) * ( oPiece.nRMax + nMargin ) )
        return FALSE;

    int     d = oPiece.nDegree;
    double  anY[ 4 ], anYY[ 7 ], anRR[ 7 ], anQ[ 7 ], anRoots[ 7 ];
    int     nRoots;
    BOOL    bLevel = std::fabs( dy ) < 1e-7;

    for ( int i = 0; i <= 3; i++ )
        anY[ i ] = oPiece.anY[ i ];
    anY[ 0 ] -= oy;
    if ( A < 1e-12 )
    {
            // A ray along the axis stays the same distance from it: the
            // piece's s where its radius is that distance. (Squared, the
            // radius would only touch zero at a pole, which bisection
            // can't find.)
        double anC[ 4 ] = { oPiece.anR[ 0 ] - std::sqrt( C ), oPiece.anR[ 1 ], oPiece.anR[ 2 ], oPiece.anR[ 3 ] };

        nRoots = Roots( anC, d, -s_nReach, 1.0 + s_nReach, anRoots );
        bLevel = FALSE;
    }
    else if ( bLevel )
    {
            // A ray across the axis at one height: the piece's s where its
            // height is the ray's (Newton below takes up the small slope).
        double anC[ 4 ] = { anY[ 0 ], anY[ 1 ], anY[ 2 ], anY[ 3 ] };

        nRoots = Roots( anC, d, -s_nReach, 1.0 + s_nReach, anRoots );
    }
    else
    {
            // dy^2 ( x^2 + z^2 - r(s)^2 ) with t = ( y(s) - oy ) / dy.
        Multiply( anY, d, anY, d, anYY );
        Multiply( oPiece.anR, d, oPiece.anR, d, anRR );
        for ( int i = 0; i <= 2 * d; i++ )
            anQ[ i ] = ( A * anYY[ i ] ) + ( ( i <= d ) ? B * dy * anY[ i ] : 0.0 ) + ( dy * dy * ( ( ( i == 0 ) ? C : 0.0 ) - anRR[ i ] ) );
        nRoots = Roots( anQ, 2 * d, -s_nReach, 1.0 + s_nReach, anRoots );
    }

    BOOL bFound = FALSE;

    for ( int n = 0; n < nRoots; n++ )
    {
        double s = anRoots[ n ];
        double r = Poly( oPiece.anR, d, s );
        double y = Poly( oPiece.anY, d, s );
        double anT[ 2 ];
        int    nT = 0;

        if ( A < 1e-12 )
            anT[ nT++ ] = ( y - oy ) / dy;
        else
        {
            double nDet = ( B * B ) - ( 4.0 * A * ( C - ( r * r ) ) );

            if ( nDet < 0.0 )
                nDet = 0.0;

            double nRoot = std::sqrt( nDet );

            anT[ nT++ ] = ( -B - nRoot ) / ( 2.0 * A );
            anT[ nT++ ] = ( -B + nRoot ) / ( 2.0 * A );
            if ( ! bLevel )
            {
                    // The one at the right height.
                if ( std::fabs( oy + ( dy * anT[ 1 ] ) - y ) < std::fabs( oy + ( dy * anT[ 0 ] ) - y ) )
                    anT[ 0 ] = anT[ 1 ];
                nT = 1;
            }
        }
        for ( int k = 0; k < nT; k++ )
        {
            double t = anT[ k ], si = s;

                // Newton on ( x^2 + z^2 - r(s)^2, oy + dy t - y(s) ).
            for ( int nStep = 0; nStep < 6; nStep++ )
            {
                double rr = Poly( oPiece.anR, d, si ), r1 = Slope( oPiece.anR, d, si );
                double yy = Poly( oPiece.anY, d, si ), y1 = Slope( oPiece.anY, d, si );
                double F1 = ( A * t * t ) + ( B * t ) + C - ( rr * rr ), F2 = oy + ( dy * t ) - yy;
                double J00 = ( 2.0 * A * t ) + B, J01 = -2.0 * rr * r1, J10 = dy, J11 = -y1;
                double nDet = ( J00 * J11 ) - ( J01 * J10 );

                if ( std::fabs( nDet ) < 1e-300 )
                    break;

                double dt = ( ( F1 * J11 ) - ( J01 * F2 ) ) / nDet, ds = ( ( J00 * F2 ) - ( J10 * F1 ) ) / nDet;

                t -= dt;
                si -= ds;
                if ( std::fabs( dt ) < 1e-15 * ( 1.0 + std::fabs( t ) ) && std::fabs( ds ) < 1e-15 )
                    break;
            }
            if ( si < -1e-6 || si > 1.0 + 1e-6 || ! ( t > nLow ) || ! ( t < nBest ) )
                continue;

            double x = ox + ( t * dx ), z = oz + ( t * dz );

            if ( ! InSweep( x, z ) )
                continue;
            nBest = t;
            bFound = TRUE;
        }
    }
    return bFound;
}

BOOL CLathe::LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT )
{
    double nBest = nHigh;
    BOOL   bFound = FALSE;

    for ( const SPiece &oPiece : m_aoPieces )
    {
        if ( PieceHit( oPiece, anOrigin, anDirection, nLow, nHigh, nBest ) )
        {
            bFound = TRUE;
            if ( m_bAnyHit )
                break;
        }
    }
    if ( m_bClosed && m_bPartial && ! ( bFound && m_bAnyHit ) )
    {
        for ( int k = 0; k < 2; k++ )
        {
            const double *n = m_anEndNormal[ k ];
            double        nDenominator = ( anDirection[ 0 ] * n[ 0 ] ) + ( anDirection[ 2 ] * n[ 2 ] );

            if ( std::fabs( nDenominator ) < 1e-15 )
                continue;

            double t = -( ( anOrigin[ 0 ] * n[ 0 ] ) + ( anOrigin[ 2 ] * n[ 2 ] ) ) / nDenominator;

            if ( ! ( t > nLow ) || ! ( t < nBest ) )
                continue;

            double x = anOrigin[ 0 ] + ( t * anDirection[ 0 ] ), y = anOrigin[ 1 ] + ( t * anDirection[ 1 ] );
            double z = anOrigin[ 2 ] + ( t * anDirection[ 2 ] );
            double r = ( x * m_anEndAlong[ k ][ 0 ] ) + ( z * m_anEndAlong[ k ][ 2 ] );

            if ( r >= 0.0 && InRegion( r, y ) )
            {
                nBest = t;
                bFound = TRUE;
            }
        }
    }
    if ( bFound )
        nT = nBest;
    return bFound;
}

double CLathe::Nearest( double r, double y, int &nPiece, double &s ) const
{
    if ( m_bNearValid && r == m_nNearR && y == m_nNearY )
    {
        nPiece = m_nNearPiece;
        s = m_nNearS;
        return m_nNearDistance;
    }

    double nBest = 1e300;

    nPiece = 0;
    s = 0.0;
    for ( size_t n = 0; n < m_aoPieces.size(); n++ )
    {
        const SPiece &oPiece = m_aoPieces[ n ];
        int           d = oPiece.nDegree;

            // A piece lies in its control points' box, so none of its points
            // can be nearer than the box. Skip it if the box is farther than
            // the best so far, allowing for rounding in how the candidates'
            // distances are worked out (so the answer is exactly what
            // looking at every piece gives).
        if ( nBest < 1e300 )
        {
            double nSlack = 1e-13 * ( std::fabs( r ) + std::fabs( y ) + std::fabs( oPiece.anR[ 0 ] ) + std::fabs( oPiece.anR[ 1 ] ) +
                                      std::fabs( oPiece.anR[ 2 ] ) + std::fabs( oPiece.anR[ 3 ] ) + std::fabs( oPiece.anY[ 0 ] ) +
                                      std::fabs( oPiece.anY[ 1 ] ) + std::fabs( oPiece.anY[ 2 ] ) + std::fabs( oPiece.anY[ 3 ] ) + 1e-300 );
            double nGapR = ( r < oPiece.nRMin ) ? oPiece.nRMin - r : ( r > oPiece.nRMax ) ? r - oPiece.nRMax : 0.0;
            double nGapY = ( y < oPiece.nYMin ) ? oPiece.nYMin - y : ( y > oPiece.nYMax ) ? y - oPiece.nYMax : 0.0;

            nGapR = ( nGapR > nSlack ) ? nGapR - nSlack : 0.0;
            nGapY = ( nGapY > nSlack ) ? nGapY - nSlack : 0.0;
            if ( ( ( nGapR * nGapR ) + ( nGapY * nGapY ) ) * ( 1.0 - 1e-12 ) > nBest )
                continue;
        }
        double        anR[ 4 ], anY[ 4 ], anR1[ 3 ], anY1[ 3 ], anA[ 6 ], anB[ 6 ], anG[ 6 ], anRoots[ 6 ];
        double        anCandidates[ 8 ] = { 0.0, 1.0 };
        int           nCandidates = 2;

        for ( int i = 0; i <= 3; i++ )
        {
            anR[ i ] = oPiece.anR[ i ];
            anY[ i ] = oPiece.anY[ i ];
        }
        anR[ 0 ] -= r;
        anY[ 0 ] -= y;
        for ( int i = 1; i <= d; i++ )
        {
            anR1[ i - 1 ] = i * oPiece.anR[ i ];
            anY1[ i - 1 ] = i * oPiece.anY[ i ];
        }

            // Where the distance stops changing: ( P - p ) . P' = 0.
        int nG = Multiply( anR, d, anR1, d - 1, anA );

        Multiply( anY, d, anY1, d - 1, anB );
        for ( int i = 0; i <= nG; i++ )
            anG[ i ] = anA[ i ] + anB[ i ];

        int nRoots = Roots( anG, nG, 0.0, 1.0, anRoots );

        for ( int k = 0; k < nRoots; k++ )
            anCandidates[ nCandidates++ ] = anRoots[ k ];
        for ( int k = 0; k < nCandidates; k++ )
        {
            double dr = Poly( anR, d, anCandidates[ k ] ), dyy = Poly( anY, d, anCandidates[ k ] );
            double nDistance = ( dr * dr ) + ( dyy * dyy );

            if ( nDistance < nBest )
            {
                nBest = nDistance;
                nPiece = (int)n;
                s = anCandidates[ k ];
            }
        }
    }
    m_nNearR = r;
    m_nNearY = y;
    m_nNearPiece = nPiece;
    m_nNearS = s;
    m_nNearDistance = std::sqrt( nBest );
    m_bNearValid = TRUE;
    return m_nNearDistance;
}

BOOL CLathe::Occludes( CRay &oRay, CObjects *pIgnore )
{
        // A shadow ray needs a hit, not the nearest. Not when the ray leaves
        // this lathe: then the nearest hit decides whether it's far enough
        // past the surface it left.
    if ( pIgnore == this )
        return CLocalShape::Occludes( oRay, pIgnore );

    CVector oHit;
    BOOL    bHit;

    m_bAnyHit = TRUE;
    bHit = Intersect( oRay, oHit, pIgnore );
    m_bAnyHit = FALSE;
    return bHit;
}

void CLathe::LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] )
{
    double r = std::sqrt( ( anPoint[ 0 ] * anPoint[ 0 ] ) + ( anPoint[ 2 ] * anPoint[ 2 ] ) );
    int    nPiece;
    double s;
    double nDistance = Nearest( r, anPoint[ 1 ], nPiece, s );

    if ( m_bClosed && m_bPartial )
    {
        for ( int k = 0; k < 2; k++ )
        {
            double nAlong = ( anPoint[ 0 ] * m_anEndAlong[ k ][ 0 ] ) + ( anPoint[ 2 ] * m_anEndAlong[ k ][ 2 ] );
            double nOff = std::fabs( ( anPoint[ 0 ] * m_anEndNormal[ k ][ 0 ] ) + ( anPoint[ 2 ] * m_anEndNormal[ k ][ 2 ] ) );

            if ( nAlong >= -1e-9 && nOff < nDistance )
            {
                anNormal[ 0 ] = m_anEndNormal[ k ][ 0 ];
                anNormal[ 1 ] = 0.0;
                anNormal[ 2 ] = m_anEndNormal[ k ][ 2 ];
                return;
            }
        }
    }

    const SPiece &oPiece = m_aoPieces[ (size_t)nPiece ];
    double        r1 = Slope( oPiece.anR, oPiece.nDegree, s ), y1 = Slope( oPiece.anY, oPiece.nDegree, s );

    if ( std::hypot( r1, y1 ) < 1e-12 )
    {
            // A piece whose end has no slope (control points on top of
            // it): the slope just inside.
        double si = ( s < 0.5 ) ? s + 1e-4 : s - 1e-4;

        r1 = Slope( oPiece.anR, oPiece.nDegree, si );
        y1 = Slope( oPiece.anY, oPiece.nDegree, si );
    }

    double nr = y1 * m_nOrient, ny = -r1 * m_nOrient;
    double c = 1.0, sn = 0.0;

    if ( r > 1e-300 )
    {
        c = anPoint[ 0 ] / r;
        sn = anPoint[ 2 ] / r;
    }
    anNormal[ 0 ] = nr * c;
    anNormal[ 1 ] = ny;
    anNormal[ 2 ] = nr * sn;
}

void CLathe::LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV )
{
    double r = std::sqrt( ( anPoint[ 0 ] * anPoint[ 0 ] ) + ( anPoint[ 2 ] * anPoint[ 2 ] ) );
    double nAngle = std::atan2( (double)anPoint[ 2 ], (double)anPoint[ 0 ] );
    int    nPiece;
    double s;

    if ( nAngle < 0.0 )
        nAngle += s_nTwoPi;
    if ( m_bPartial && nAngle > m_nSweep )
        nAngle = ( nAngle - m_nSweep < s_nTwoPi - nAngle ) ? m_nSweep : 0.0;
    Nearest( r, anPoint[ 1 ], nPiece, s );
    nU = nAngle / m_nSweep;

    const SPiece &oPiece = m_aoPieces[ (size_t)nPiece ];

    nV = oPiece.nV0 + ( s * ( oPiece.nV1 - oPiece.nV0 ) );
}

void CLathe::LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] )
{
    anLow[ 0 ] = -m_nRMax;
    anLow[ 1 ] = m_nYMin;
    anLow[ 2 ] = -m_nRMax;
    anHigh[ 0 ] = m_nRMax;
    anHigh[ 1 ] = m_nYMax;
    anHigh[ 2 ] = m_nRMax;
}

void CLathe::LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals )
{
    int nAround = (int)std::ceil( nSegments * m_nSweep / s_nTwoPi );

    if ( nAround < 4 )
        nAround = 4;
    for ( const SPiece &oPiece : m_aoPieces )
    {
        int                 nSteps = ( oPiece.nDegree == 1 ) ? 1 : 8;
        std::vector<double> anProfile;      // r, y, nr, ny at each step

        for ( int j = 0; j <= nSteps; j++ )
        {
            double s = (double)j / nSteps;
            double r1 = Slope( oPiece.anR, oPiece.nDegree, s ), y1 = Slope( oPiece.anY, oPiece.nDegree, s );
            double l = std::hypot( r1, y1 );

            if ( l < 1e-12 )
            {
                double si = ( s < 0.5 ) ? s + 1e-4 : s - 1e-4;

                r1 = Slope( oPiece.anR, oPiece.nDegree, si );
                y1 = Slope( oPiece.anY, oPiece.nDegree, si );
                l = std::hypot( r1, y1 );
            }
            anProfile.push_back( Poly( oPiece.anR, oPiece.nDegree, s ) );
            anProfile.push_back( Poly( oPiece.anY, oPiece.nDegree, s ) );
            anProfile.push_back( ( l > 0.0 ) ? y1 * m_nOrient / l : 0.0 );
            anProfile.push_back( ( l > 0.0 ) ? -r1 * m_nOrient / l : 1.0 );
        }
        for ( int i = 0; i < nAround; i++ )
        {
            double a0 = m_nSweep * i / nAround, a1 = m_nSweep * ( i + 1 ) / nAround;
            double c0 = std::cos( a0 ), s0 = std::sin( a0 ), c1 = std::cos( a1 ), s1 = std::sin( a1 );

            for ( int j = 0; j < nSteps; j++ )
            {
                const double *p = &anProfile[ (size_t)j * 4 ], *q = &anProfile[ (size_t)( j + 1 ) * 4 ];
                FLT A[ 3 ] = { p[ 0 ] * c0, p[ 1 ], p[ 0 ] * s0 }, nA[ 3 ] = { p[ 2 ] * c0, p[ 3 ], p[ 2 ] * s0 };
                FLT B[ 3 ] = { p[ 0 ] * c1, p[ 1 ], p[ 0 ] * s1 }, nB[ 3 ] = { p[ 2 ] * c1, p[ 3 ], p[ 2 ] * s1 };
                FLT C[ 3 ] = { q[ 0 ] * c1, q[ 1 ], q[ 0 ] * s1 }, nC[ 3 ] = { q[ 2 ] * c1, q[ 3 ], q[ 2 ] * s1 };
                FLT D[ 3 ] = { q[ 0 ] * c0, q[ 1 ], q[ 0 ] * s0 }, nD[ 3 ] = { q[ 2 ] * c0, q[ 3 ], q[ 2 ] * s0 };

                LocalMesh::Add( anPositions, anNormals, A, B, C, nA, nB, nC );
                LocalMesh::Add( anPositions, anNormals, A, C, D, nA, nC, nD );
            }
        }
    }
    if ( m_bClosed && m_bPartial )
    {
        std::vector<Outline::Polygon> aoOutlines( 1, m_oRegion );
        Outline::SShape               oShape;
        std::string                   strError;

        if ( Outline::Fill( aoOutlines, oShape, strError ) )
        {
            for ( int k = 0; k < 2; k++ )
            {
                const double *u = m_anEndAlong[ k ], *n = m_anEndNormal[ k ];
                FLT           nN[ 3 ] = { n[ 0 ], n[ 1 ], n[ 2 ] };

                for ( size_t t = 0; t + 2 < oShape.anTriangles.size(); t += 3 )
                {
                    FLT P[ 3 ][ 3 ];

                    for ( int c = 0; c < 3; c++ )
                    {
                        const Outline::SPoint &p = oShape.aoPoints[ (size_t)oShape.anTriangles[ t + (size_t)c ] ];

                        P[ c ][ 0 ] = p.x * u[ 0 ];
                        P[ c ][ 1 ] = p.y;
                        P[ c ][ 2 ] = p.x * u[ 2 ];
                    }
                    LocalMesh::Add( anPositions, anNormals, P[ 0 ], P[ 1 ], P[ 2 ], nN, nN, nN );
                }
            }
        }
    }
}

const char *const *CLathe::ShapeFields( void )
{
    static const char *const astrFields[] = { "profile", "smooth", "capped", "sweep", NULL };

    return astrFields;
}

void CLathe::ShapeToJson( IASLib::CObjectNode &oNode )
{
    Profile::Write( *oNode.putArray( "profile" ), m_oProfile );
    if ( m_bSmooth )
        RayJson::PutBool( oNode, "smooth", m_bSmooth );
    if ( m_bCapped )
        RayJson::PutBool( oNode, "capped", m_bCapped );
    if ( m_bPartial )
        RayJson::PutNumber( oNode, "sweep", m_nSweep * 180.0 / PI );
}

BOOL CLathe::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    FLT nSweep = m_nSweep * 180.0 / PI;

    if ( ! RayJson::GetBool( oNode, "smooth", m_bSmooth, FALSE ) ||
         ! RayJson::GetBool( oNode, "capped", m_bCapped, FALSE ) ||
         ! RayJson::GetNumber( oNode, "sweep", nSweep, FALSE ) )
        return FALSE;
    if ( ! ( nSweep > 0.0 ) || nSweep > 360.0 + 1e-9 )
        return RayJson::Fail( "sweep must be more than 0 and at most 360 degrees" );
    m_nSweep = ( nSweep >= 360.0 ) ? 2.0 * PI : nSweep * PI / 180.0;

    const IASLib::CJsonNode *pProfile = RayJson::Field( oNode, "profile", TRUE );

    if ( ! pProfile )
        return FALSE;

    RayJson::CWhere oWhere( "profile" );

    return Profile::Read( *pProfile, false, m_bSmooth ? true : false, 1, m_oProfile );
}

void CLathe::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CLathe::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
