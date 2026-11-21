/*************************************************************************
*   CBlob                                                                *
*                                                                        *
*   Metaballs. See Blob.h.                                               *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Blob.h"
#include "SceneJson.h"

#include <algorithm>
#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CBlob, "blob" );

CBlob::CBlob( void ) :
    m_nThreshold( 0.5 )
{
    SBall oBall = { { 0.0, 0.0, 0.0 }, 1.0, 1.0 };

    m_aoBalls.push_back( oBall );
    Prepare();
}

CBlob::~CBlob( void )
{
}

BOOL CBlob::Set( const CVector &oCenter, const std::vector<SBall> &aoBalls, FLT nThreshold )
{
    m_oCenter = oCenter;
    m_aoBalls = aoBalls;
    m_nThreshold = nThreshold;
    return Prepare();
}

BOOL CBlob::PrepareField( void )
{
    BOOL bPositive = FALSE;

    if ( ! ( m_nThreshold > 0.0 ) )
        return RayJson::Fail( "threshold must be more than 0" );
    if ( m_aoBalls.empty() )
        return RayJson::Fail( "balls must have at least one ball" );
    for ( size_t n = 0; n < m_aoBalls.size(); n++ )
    {
        const SBall &oBall = m_aoBalls[ n ];

        if ( ! ( oBall.nRadius > 0.0 ) )
        {
            RayJson::CWhere oWhere( "balls", (int)n );

            return RayJson::Fail( "radius must be more than 0" );
        }
        if ( oBall.nStrength > 0.0 )
        {
            for ( int i = 0; i < 3; i++ )
            {
                double nLow = oBall.anCenter[ i ] - oBall.nRadius, nHigh = oBall.anCenter[ i ] + oBall.nRadius;

                m_anLow[ i ] = ( ! bPositive || nLow < m_anLow[ i ] ) ? nLow : m_anLow[ i ];
                m_anHigh[ i ] = ( ! bPositive || nHigh > m_anHigh[ i ] ) ? nHigh : m_anHigh[ i ];
            }
            bPositive = TRUE;
        }
    }
    if ( ! bPositive )
        return RayJson::Fail( "balls must have at least one ball of positive strength" );
    return TRUE;
}

double CBlob::Sum( const double anPoint[ 3 ] ) const
{
    double nSum = 0.0;

    for ( size_t n = 0; n < m_aoBalls.size(); n++ )
    {
        const SBall &oBall = m_aoBalls[ n ];
        double       dx = anPoint[ 0 ] - oBall.anCenter[ 0 ];
        double       dy = anPoint[ 1 ] - oBall.anCenter[ 1 ];
        double       dz = anPoint[ 2 ] - oBall.anCenter[ 2 ];
        double       q = ( ( dx * dx ) + ( dy * dy ) + ( dz * dz ) ) / ( oBall.nRadius * oBall.nRadius );

        if ( q < 1.0 )
        {
            double u = 1.0 - q;

            nSum += oBall.nStrength * u * u * u;
        }
    }
    return nSum;
}

double CBlob::Field( const double anPoint[ 3 ] ) const
{
    return m_nThreshold - Sum( anPoint );
}

void CBlob::FieldBounds( double anLow[ 3 ], double anHigh[ 3 ] ) const
{
    for ( int i = 0; i < 3; i++ )
    {
        anLow[ i ] = m_anLow[ i ];
        anHigh[ i ] = m_anHigh[ i ];
    }
}

namespace
{
    struct SEvent
    {
        double  t;
        int     nBall;
    };

    bool EarlierEvent( const SEvent &a, const SEvent &b )
    {
        return ( a.t < b.t ) || ( a.t == b.t && a.nBall < b.nBall );
    }
}

BOOL CBlob::FieldHit( const double anOrigin[ 3 ], const double anDirection[ 3 ], double nLow, double nHigh, double &nT ) const
{
    double nFrom = nLow, nTo = nHigh;

    if ( ! ClipToBox( anOrigin, anDirection, m_anLow, m_anHigh, nFrom, nTo ) )
    {
            // Inside only negative balls the sum can't reach the threshold,
            // but a ray may start inside the box and leave it.
        return FALSE;
    }

        // Where the ray enters and leaves each ball it crosses.
    std::vector<SEvent> aoEvents;
    std::vector<double> anB( m_aoBalls.size() ), anC( m_aoBalls.size() );

    for ( size_t n = 0; n < m_aoBalls.size(); n++ )
    {
        const SBall &oBall = m_aoBalls[ n ];
        double       o[ 3 ] = { anOrigin[ 0 ] - oBall.anCenter[ 0 ], anOrigin[ 1 ] - oBall.anCenter[ 1 ],
                                anOrigin[ 2 ] - oBall.anCenter[ 2 ] };
        double       b = ( o[ 0 ] * anDirection[ 0 ] ) + ( o[ 1 ] * anDirection[ 1 ] ) + ( o[ 2 ] * anDirection[ 2 ] );
        double       c = ( o[ 0 ] * o[ 0 ] ) + ( o[ 1 ] * o[ 1 ] ) + ( o[ 2 ] * o[ 2 ] );
        double       nDet = ( b * b ) - c + ( oBall.nRadius * oBall.nRadius );

        anB[ n ] = b;
        anC[ n ] = c;
        if ( nDet <= 0.0 )
            continue;

        double nRoot = sqrt( nDet );
        SEvent oIn = { -b - nRoot, (int)n }, oOut = { -b + nRoot, -1 - (int)n };

        if ( oOut.t <= nLow || oIn.t >= nHigh )
            continue;
        aoEvents.push_back( oIn );
        aoEvents.push_back( oOut );
    }
    if ( aoEvents.empty() )
        return FALSE;
    std::sort( aoEvents.begin(), aoEvents.end(), EarlierEvent );

        // Between each pair of events the same balls count: their sum is one
        // polynomial in s = t - (the stretch's start).
    std::vector<int> anActive;

    for ( size_t e = 0; e + 1 < aoEvents.size(); e++ )
    {
        const SEvent &oEvent = aoEvents[ e ];

        if ( oEvent.nBall >= 0 )
            anActive.push_back( oEvent.nBall );
        else
            anActive.erase( std::remove( anActive.begin(), anActive.end(), -1 - oEvent.nBall ), anActive.end() );

        double tStart = oEvent.t, tEnd = aoEvents[ e + 1 ].t;

        if ( anActive.empty() || tEnd <= nLow || tEnd <= tStart )
            continue;
        if ( tStart >= nHigh )
            break;

        double anPoly[ 7 ] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

        for ( size_t a = 0; a < anActive.size(); a++ )
        {
            int          n = anActive[ a ];
            const SBall &oBall = m_aoBalls[ n ];
            double       nAlpha = 1.0 / ( oBall.nRadius * oBall.nRadius );
            double       nBeta = tStart + anB[ n ];
            double       nGamma = ( tStart * tStart ) + ( 2.0 * anB[ n ] * tStart ) + anC[ n ];
                // 1 - q = u0 + u1 s + u2 s^2, then cubed.
            double       u[ 3 ] = { 1.0 - ( nAlpha * nGamma ), -2.0 * nAlpha * nBeta, -nAlpha };
            double       u2[ 5 ] = { 0.0, 0.0, 0.0, 0.0, 0.0 };
            double       u3[ 7 ] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

            for ( int i = 0; i < 3; i++ )
                for ( int j = 0; j < 3; j++ )
                    u2[ i + j ] += u[ i ] * u[ j ];
            for ( int i = 0; i < 5; i++ )
                for ( int j = 0; j < 3; j++ )
                    u3[ i + j ] += u2[ i ] * u[ j ];
            for ( int i = 0; i < 7; i++ )
                anPoly[ i ] += oBall.nStrength * u3[ i ];
        }
        anPoly[ 0 ] -= m_nThreshold;

        double nFromS = ( nLow > tStart ) ? nLow - tStart : 0.0;
        double nToS = ( ( nHigh < tEnd ) ? nHigh : tEnd ) - tStart;
        double anRoots[ 6 ];
        int    nRoots = PolynomialRoots( anPoly, 6, nFromS, nToS, anRoots );

        for ( int r = 0; r < nRoots; r++ )
        {
            double t = anRoots[ r ] + tStart;

            if ( t > nLow && t < nHigh )
            {
                nT = t;
                return TRUE;
            }
        }
    }
    return FALSE;
}

void CBlob::FieldNormal( const double anPoint[ 3 ], double anNormal[ 3 ] ) const
{
        // The field is threshold - sum, so its gradient, outward, is minus
        // the sum's: each ball adds 6 s (1 - q)^2 (p - c) / R^2.
    anNormal[ 0 ] = anNormal[ 1 ] = anNormal[ 2 ] = 0.0;
    for ( size_t n = 0; n < m_aoBalls.size(); n++ )
    {
        const SBall &oBall = m_aoBalls[ n ];
        double       d[ 3 ] = { anPoint[ 0 ] - oBall.anCenter[ 0 ], anPoint[ 1 ] - oBall.anCenter[ 1 ],
                                anPoint[ 2 ] - oBall.anCenter[ 2 ] };
        double       nAlpha = 1.0 / ( oBall.nRadius * oBall.nRadius );
        double       q = ( ( d[ 0 ] * d[ 0 ] ) + ( d[ 1 ] * d[ 1 ] ) + ( d[ 2 ] * d[ 2 ] ) ) * nAlpha;

        if ( q < 1.0 )
        {
            double nWeight = 6.0 * oBall.nStrength * ( 1.0 - q ) * ( 1.0 - q ) * nAlpha;

            for ( int i = 0; i < 3; i++ )
                anNormal[ i ] += nWeight * d[ i ];
        }
    }
}

const char *const *CBlob::ShapeFields( void )
{
    static const char *const astrFields[] = { "balls", "threshold", "scale", NULL };

    return astrFields;
}

void CBlob::FieldToJson( IASLib::CObjectNode &oNode )
{
    IASLib::CArrayNode *pBalls = oNode.putArray( "balls" );

    for ( size_t n = 0; n < m_aoBalls.size(); n++ )
    {
        const SBall         &oBall = m_aoBalls[ n ];
        IASLib::CObjectNode *pBall = pBalls->addObject();

        RayJson::PutVector( *pBall, "center", CVector( oBall.anCenter[ 0 ], oBall.anCenter[ 1 ], oBall.anCenter[ 2 ] ) );
        RayJson::PutNumber( *pBall, "radius", oBall.nRadius );
        if ( oBall.nStrength != 1.0 )
            RayJson::PutNumber( *pBall, "strength", oBall.nStrength );
    }
    if ( m_nThreshold != 0.5 )
        RayJson::PutNumber( oNode, "threshold", m_nThreshold );
}

BOOL CBlob::FieldFromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrBall[] = { "center", "radius", "strength", NULL };
    const IASLib::CJsonNode *pBalls = RayJson::Field( oNode, "balls", TRUE );

    m_nThreshold = 0.5;
    m_aoBalls.clear();
    if ( pBalls == NULL )
        return FALSE;
    if ( ! pBalls->isArray() )
    {
        RayJson::CWhere oWhere( "balls" );

        return RayJson::Fail( "balls must be an array of balls" );
    }
    for ( int n = 0; n < pBalls->size(); n++ )
    {
        const IASLib::CJsonNode *pBall = pBalls->get( n );
        RayJson::CWhere          oWhere( "balls", n );
        CVector                  oCenter;
        FLT                      nRadius = 0.0, nStrength = 1.0;

        if ( ! pBall->isObject() )
            return RayJson::Fail( "a ball must be an object with center and radius" );
        if ( ! RayJson::OnlyFields( *pBall, astrBall ) || ! RayJson::GetVector( *pBall, "center", oCenter, TRUE ) ||
             ! RayJson::GetNumber( *pBall, "radius", nRadius, TRUE ) || ! RayJson::GetNumber( *pBall, "strength", nStrength, FALSE ) )
            return FALSE;

        SBall oNew = { { oCenter.GetX(), oCenter.GetY(), oCenter.GetZ() }, nRadius, nStrength };

        m_aoBalls.push_back( oNew );
    }
    return RayJson::GetNumber( oNode, "threshold", m_nThreshold, FALSE );
}

void CBlob::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CBlob::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
