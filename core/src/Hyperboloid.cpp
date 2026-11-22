/*************************************************************************
*   CHyperboloid                                                         *
*                                                                        *
*   A hyperboloid of one sheet. See Hyperboloid.h.                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Hyperboloid.h"
#include "SceneJson.h"

#include <math.h>


DYNAMIC_IMPLEMENTS( CHyperboloid, "hyperboloid" );

CHyperboloid::CHyperboloid( void ) :
    m_nRadius( 0.5 ),
    m_nTopRadius( 1.0 ),
    m_nHeight( 2.0 ),
    m_bCapped( FALSE ),
    m_nK( 0.75 )
{
    Prepare();
}

CHyperboloid::~CHyperboloid( void )
{
}

BOOL CHyperboloid::PrepareShape( void )
{
    if ( ! ( m_nRadius > 0.0 ) )
        return RayJson::Fail( "the radius must be more than 0" );
    if ( ! ( m_nTopRadius >= m_nRadius ) )
        return RayJson::Fail( "topRadius must be at least the radius" );
    if ( ! ( m_nHeight > 0.0 ) )
        return RayJson::Fail( "the height must be more than 0" );

    FLT nHalf = m_nHeight / 2.0;

    m_nK = ( ( m_nTopRadius * m_nTopRadius ) - ( m_nRadius * m_nRadius ) ) / ( nHalf * nHalf );
    return TRUE;
}

int CHyperboloid::CapAt( const FLT anPoint[ 3 ] ) const
{
    if ( ! m_bCapped )
        return 0;

    FLT nNear = 1e-7 * ( m_nHeight + m_nTopRadius );

    if ( ABS( anPoint[ 1 ] - ( m_nHeight / 2.0 ) ) <= nNear )
        return 1;
    if ( ABS( anPoint[ 1 ] + ( m_nHeight / 2.0 ) ) <= nNear )
        return -1;
    return 0;
}

BOOL CHyperboloid::LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT )
{
    const FLT *o = anOrigin, *d = anDirection;
    double     nHalf = m_nHeight / 2.0;
    double     a = ( d[ 0 ] * d[ 0 ] ) + ( d[ 2 ] * d[ 2 ] ) - ( m_nK * d[ 1 ] * d[ 1 ] );
    double     b = 2.0 * ( ( o[ 0 ] * d[ 0 ] ) + ( o[ 2 ] * d[ 2 ] ) - ( m_nK * o[ 1 ] * d[ 1 ] ) );
    double     c = ( o[ 0 ] * o[ 0 ] ) + ( o[ 2 ] * o[ 2 ] ) - ( m_nK * o[ 1 ] * o[ 1 ] ) - ( m_nRadius * m_nRadius );
    double     anRoots[ 2 ];
    int        nRoots = 0;
    double     nBest = nHigh;
    BOOL       bFound = FALSE;

    if ( ABS( a ) > 1e-14 )
    {
        double nDet = ( b * b ) - ( 4.0 * a * c );

        if ( nDet >= 0.0 )
        {
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

        if ( t > nLow && t < nBest && y >= -nHalf && y <= nHalf )
        {
            nBest = t;
            bFound = TRUE;
        }
    }

    if ( m_bCapped && d[ 1 ] != 0.0 )
    {
        for ( int nEnd = -1; nEnd <= 1; nEnd += 2 )
        {
            double t = ( ( nEnd * nHalf ) - o[ 1 ] ) / d[ 1 ];
            double x = o[ 0 ] + ( t * d[ 0 ] ), z = o[ 2 ] + ( t * d[ 2 ] );

            if ( t > nLow && t < nBest && ( ( x * x ) + ( z * z ) ) <= m_nTopRadius * m_nTopRadius )
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

void CHyperboloid::LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] )
{
    int nCap = CapAt( anPoint );

    if ( nCap != 0 )
    {
        anNormal[ 0 ] = 0.0;
        anNormal[ 1 ] = (FLT)nCap;
        anNormal[ 2 ] = 0.0;
        return;
    }
    anNormal[ 0 ] = anPoint[ 0 ];
    anNormal[ 1 ] = -m_nK * anPoint[ 1 ];
    anNormal[ 2 ] = anPoint[ 2 ];
}

void CHyperboloid::LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV )
{
    int nCap = CapAt( anPoint );

    if ( nCap != 0 )
    {
        nU = 0.5 + ( anPoint[ 0 ] / ( 2.0 * m_nTopRadius ) );
        nV = 0.5 + ( nCap * anPoint[ 2 ] / ( 2.0 * m_nTopRadius ) );
        return;
    }
    nU = 0.5 + ( (FLT)atan2( (double)anPoint[ 2 ], (double)anPoint[ 0 ] ) / ( 2.0 * PI ) );
    nV = ( anPoint[ 1 ] / m_nHeight ) + 0.5;
}

void CHyperboloid::LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] )
{
    anLow[ 0 ] = -m_nTopRadius;
    anLow[ 1 ] = -m_nHeight / 2.0;
    anLow[ 2 ] = -m_nTopRadius;
    anHigh[ 0 ] = m_nTopRadius;
    anHigh[ 1 ] = m_nHeight / 2.0;
    anHigh[ 2 ] = m_nTopRadius;
}

void CHyperboloid::LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals )
{
    std::vector<FLT> anProfile;
    int              nSteps = ( nSegments / 2 > 6 ) ? nSegments / 2 : 6;

    for ( int n = 0; n <= nSteps; n++ )
    {
        FLT nY = m_nHeight * ( ( (FLT)n / nSteps ) - 0.5 );
        FLT nR = SQRT( ( m_nRadius * m_nRadius ) + ( m_nK * nY * nY ) );
        FLT nLength = SQRT( ( nR * nR ) + ( m_nK * m_nK * nY * nY ) );

        anProfile.push_back( nR );
        anProfile.push_back( nY );
        anProfile.push_back( nR / nLength );
        anProfile.push_back( -m_nK * nY / nLength );
    }
    LocalMesh::Revolve( anPositions, anNormals, anProfile, nSegments );
    if ( m_bCapped )
    {
        LocalMesh::Ring( anPositions, anNormals, m_nHeight / 2.0, 0.0, m_nTopRadius, TRUE, nSegments );
        LocalMesh::Ring( anPositions, anNormals, -m_nHeight / 2.0, 0.0, m_nTopRadius, FALSE, nSegments );
    }
}

const char *const *CHyperboloid::ShapeFields( void )
{
    static const char *const astrFields[] = { "radius", "topRadius", "height", "capped", NULL };

    return astrFields;
}

void CHyperboloid::ShapeToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutNumber( oNode, "radius", m_nRadius );
    RayJson::PutNumber( oNode, "topRadius", m_nTopRadius );
    RayJson::PutNumber( oNode, "height", m_nHeight );
    RayJson::PutBool( oNode, "capped", m_bCapped );
}

BOOL CHyperboloid::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    m_bCapped = FALSE;
    return RayJson::GetNumber( oNode, "radius", m_nRadius, TRUE ) &&
           RayJson::GetNumber( oNode, "topRadius", m_nTopRadius, TRUE ) &&
           RayJson::GetNumber( oNode, "height", m_nHeight, TRUE ) &&
           RayJson::GetBool( oNode, "capped", m_bCapped, FALSE );
}

void CHyperboloid::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CHyperboloid::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
