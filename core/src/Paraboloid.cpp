/*************************************************************************
*   CParaboloid                                                          *
*                                                                        *
*   A bowl. See Paraboloid.h.                                            *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Paraboloid.h"
#include "SceneJson.h"

#include <math.h>


DYNAMIC_IMPLEMENTS( CParaboloid, "paraboloid" );

CParaboloid::CParaboloid( void ) :
    m_nRadius( 1.0 ),
    m_nHeight( 1.0 ),
    m_bCapped( FALSE ),
    m_nK( 1.0 )
{
    Prepare();
}

CParaboloid::~CParaboloid( void )
{
}

BOOL CParaboloid::PrepareShape( void )
{
    if ( ! ( m_nRadius > 0.0 ) )
        return RayJson::Fail( "the radius must be more than 0" );
    if ( ! ( m_nHeight > 0.0 ) )
        return RayJson::Fail( "the height must be more than 0" );
    m_nK = ( m_nRadius * m_nRadius ) / m_nHeight;
    return TRUE;
}

BOOL CParaboloid::OnCap( const FLT anPoint[ 3 ] ) const
{
    return ( m_bCapped && ABS( anPoint[ 1 ] - m_nHeight ) <= 1e-7 * ( m_nHeight + m_nRadius ) ) ? TRUE : FALSE;
}

BOOL CParaboloid::LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT )
{
    const FLT *o = anOrigin, *d = anDirection;
    double     a = ( d[ 0 ] * d[ 0 ] ) + ( d[ 2 ] * d[ 2 ] );
    double     b = 2.0 * ( ( o[ 0 ] * d[ 0 ] ) + ( o[ 2 ] * d[ 2 ] ) ) - ( m_nK * d[ 1 ] );
    double     c = ( o[ 0 ] * o[ 0 ] ) + ( o[ 2 ] * o[ 2 ] ) - ( m_nK * o[ 1 ] );
    double     anRoots[ 2 ];
    int        nRoots = 0;
    double     nBest = nHigh;
    BOOL       bFound = FALSE;

    if ( ABS( a ) > 1e-14 )
    {
        double nDet = ( b * b ) - ( 4.0 * a * c );

        if ( nDet >= 0.0 )
        {
                // The form that keeps precision when b dwarfs a c.
            double q = -0.5 * ( b + ( ( b >= 0.0 ) ? sqrt( nDet ) : -sqrt( nDet ) ) );

            anRoots[ nRoots++ ] = q / a;
            if ( q != 0.0 )
                anRoots[ nRoots++ ] = c / q;
        }
    }
    else if ( b != 0.0 )
        anRoots[ nRoots++ ] = -c / b;

    for ( int n = 0; n < nRoots; n++ )
    {
        double t = anRoots[ n ];
        double y = o[ 1 ] + ( t * d[ 1 ] );

        if ( t > nLow && t < nBest && y >= 0.0 && y <= m_nHeight )
        {
            nBest = t;
            bFound = TRUE;
        }
    }

    if ( m_bCapped && d[ 1 ] != 0.0 )
    {
        double t = ( m_nHeight - o[ 1 ] ) / d[ 1 ];
        double x = o[ 0 ] + ( t * d[ 0 ] ), z = o[ 2 ] + ( t * d[ 2 ] );

        if ( t > nLow && t < nBest && ( ( x * x ) + ( z * z ) ) <= m_nRadius * m_nRadius )
        {
            nBest = t;
            bFound = TRUE;
        }
    }
    if ( bFound )
        nT = nBest;
    return bFound;
}

void CParaboloid::LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] )
{
    if ( OnCap( anPoint ) )
    {
        anNormal[ 0 ] = 0.0;
        anNormal[ 1 ] = 1.0;
        anNormal[ 2 ] = 0.0;
        return;
    }
    anNormal[ 0 ] = 2.0 * anPoint[ 0 ];
    anNormal[ 1 ] = -m_nK;
    anNormal[ 2 ] = 2.0 * anPoint[ 2 ];
}

void CParaboloid::LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV )
{
    if ( OnCap( anPoint ) )
    {
        nU = 0.5 + ( anPoint[ 0 ] / ( 2.0 * m_nRadius ) );
        nV = 0.5 + ( anPoint[ 2 ] / ( 2.0 * m_nRadius ) );
        return;
    }
    nU = 0.5 + ( (FLT)atan2( (double)anPoint[ 2 ], (double)anPoint[ 0 ] ) / ( 2.0 * PI ) );
    nV = anPoint[ 1 ] / m_nHeight;
}

void CParaboloid::LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] )
{
    anLow[ 0 ] = -m_nRadius;
    anLow[ 1 ] = 0.0;
    anLow[ 2 ] = -m_nRadius;
    anHigh[ 0 ] = m_nRadius;
    anHigh[ 1 ] = m_nHeight;
    anHigh[ 2 ] = m_nRadius;
}

void CParaboloid::LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals )
{
    std::vector<FLT> anProfile;
    int              nSteps = ( nSegments / 2 > 6 ) ? nSegments / 2 : 6;

    for ( int n = 0; n <= nSteps; n++ )
    {
        FLT nR = m_nRadius * n / nSteps;
        FLT nY = ( nR * nR ) / m_nK;
        FLT nLength = SQRT( ( 4.0 * nR * nR ) + ( m_nK * m_nK ) );

        anProfile.push_back( nR );
        anProfile.push_back( nY );
        anProfile.push_back( 2.0 * nR / nLength );
        anProfile.push_back( -m_nK / nLength );
    }
    LocalMesh::Revolve( anPositions, anNormals, anProfile, nSegments );
    if ( m_bCapped )
        LocalMesh::Ring( anPositions, anNormals, m_nHeight, 0.0, m_nRadius, TRUE, nSegments );
}

const char *const *CParaboloid::ShapeFields( void )
{
    static const char *const astrFields[] = { "radius", "height", "capped", NULL };

    return astrFields;
}

void CParaboloid::ShapeToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutNumber( oNode, "radius", m_nRadius );
    RayJson::PutNumber( oNode, "height", m_nHeight );
    RayJson::PutBool( oNode, "capped", m_bCapped );
}

BOOL CParaboloid::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    m_bCapped = FALSE;
    return RayJson::GetNumber( oNode, "radius", m_nRadius, TRUE ) &&
           RayJson::GetNumber( oNode, "height", m_nHeight, TRUE ) &&
           RayJson::GetBool( oNode, "capped", m_bCapped, FALSE );
}

void CParaboloid::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CParaboloid::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
