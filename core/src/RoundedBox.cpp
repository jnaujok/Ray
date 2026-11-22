/*************************************************************************
*   CRoundedBox                                                          *
*                                                                        *
*   A box with rounded edges and corners. See RoundedBox.h.              *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "RoundedBox.h"
#include "SceneJson.h"

#include <math.h>


DYNAMIC_IMPLEMENTS( CRoundedBox, "roundedBox" );

CRoundedBox::CRoundedBox( void ) :
    m_oSize( 1.0, 1.0, 1.0 ),
    m_nRadius( 0.1 )
{
    Prepare();
}

CRoundedBox::~CRoundedBox( void )
{
}

BOOL CRoundedBox::Set( const CVector &oCenter, const CVector &oSize, FLT nRadius, const CVector &oRotationDegrees )
{
    m_oCenter = oCenter;
    m_oSize = oSize;
    m_nRadius = nRadius;
    m_oRotation = oRotationDegrees;
    return Prepare();
}

BOOL CRoundedBox::PrepareShape( void )
{
    if ( ! ( m_oSize.GetX() > 0.0 && m_oSize.GetY() > 0.0 && m_oSize.GetZ() > 0.0 ) )
        return RayJson::Fail( "every size must be more than 0" );
    m_anHalf[ 0 ] = m_oSize.GetX() / 2.0;
    m_anHalf[ 1 ] = m_oSize.GetY() / 2.0;
    m_anHalf[ 2 ] = m_oSize.GetZ() / 2.0;

    FLT nSmallest = m_anHalf[ 0 ];

    for ( int nAxis = 1; nAxis < 3; nAxis++ )
        nSmallest = ( m_anHalf[ nAxis ] < nSmallest ) ? m_anHalf[ nAxis ] : nSmallest;
    if ( ! ( m_nRadius > 0.0 && m_nRadius <= nSmallest ) )
        return RayJson::Fail( "the radius must be more than 0 and at most half the smallest size" );
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
        m_anInner[ nAxis ] = m_anHalf[ nAxis ] - m_nRadius;
    return TRUE;
}

    // Widens [nEnter, nLeave] to hold [t1, t2] (when t1 < t2).
static void Widen( double t1, double t2, double &nEnter, double &nLeave, BOOL &bAny )
{
    if ( ! ( t2 > t1 ) )
        return;
    if ( ! bAny || t1 < nEnter )
        nEnter = t1;
    if ( ! bAny || t2 > nLeave )
        nLeave = t2;
    bAny = TRUE;
}

    // The stretch of a unit ray inside the box of half sizes anHalf about the
    // origin; FALSE if it misses.
static BOOL Slabs( const FLT o[ 3 ], const FLT d[ 3 ], const double anHalf[ 3 ], double &t1, double &t2 )
{
    t1 = -1e300;
    t2 = 1e300;
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        if ( d[ nAxis ] == 0.0 )
        {
            if ( ABS( o[ nAxis ] ) > anHalf[ nAxis ] )
                return FALSE;
            continue;
        }

        double a = ( -anHalf[ nAxis ] - o[ nAxis ] ) / d[ nAxis ];
        double b = ( anHalf[ nAxis ] - o[ nAxis ] ) / d[ nAxis ];

        if ( a > b )
        {
            double nSwap = a;

            a = b;
            b = nSwap;
        }
        t1 = ( a > t1 ) ? a : t1;
        t2 = ( b < t2 ) ? b : t2;
    }
    return ( t2 > t1 ) ? TRUE : FALSE;
}

BOOL CRoundedBox::LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT )
{
    const FLT *o = anOrigin, *d = anDirection;
    double     r = m_nRadius;
    double     nEnter = 0.0, nLeave = 0.0;
    BOOL       bAny = FALSE;
    double     t1, t2;

        // The three slabs: the inner box grown by the radius along one axis.
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        double anHalf[ 3 ] = { m_anInner[ 0 ], m_anInner[ 1 ], m_anInner[ 2 ] };

        anHalf[ nAxis ] += r;
        if ( Slabs( o, d, anHalf, t1, t2 ) )
            Widen( t1, t2, nEnter, nLeave, bAny );
    }

        // The twelve edge cylinders, along each axis at each corner of the
        // inner box's cross-section.
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        int    j = ( nAxis + 1 ) % 3, k = ( nAxis + 2 ) % 3;
        double a = ( d[ j ] * d[ j ] ) + ( d[ k ] * d[ k ] );

        if ( a < 1e-300 )
            continue;
        for ( int nCorner = 0; nCorner < 4; nCorner++ )
        {
            double cj = ( nCorner & 1 ) ? m_anInner[ j ] : -m_anInner[ j ];
            double ck = ( nCorner & 2 ) ? m_anInner[ k ] : -m_anInner[ k ];
            double oj = o[ j ] - cj, ok = o[ k ] - ck;
            double b = ( oj * d[ j ] ) + ( ok * d[ k ] );
            double c = ( oj * oj ) + ( ok * ok ) - ( r * r );
            double nDet = ( b * b ) - ( a * c );

            if ( nDet < 0.0 )
                continue;

            double nRoot = sqrt( nDet );
            double s1 = ( -b - nRoot ) / a, s2 = ( -b + nRoot ) / a;

                // Clipped to the edge's length along its axis.
            if ( d[ nAxis ] != 0.0 )
            {
                double a1 = ( -m_anInner[ nAxis ] - o[ nAxis ] ) / d[ nAxis ];
                double a2 = ( m_anInner[ nAxis ] - o[ nAxis ] ) / d[ nAxis ];

                if ( a1 > a2 )
                {
                    double nSwap = a1;

                    a1 = a2;
                    a2 = nSwap;
                }
                s1 = ( a1 > s1 ) ? a1 : s1;
                s2 = ( a2 < s2 ) ? a2 : s2;
            }
            else if ( ABS( o[ nAxis ] ) > m_anInner[ nAxis ] )
                continue;
            Widen( s1, s2, nEnter, nLeave, bAny );
        }
    }

        // The eight corner spheres.
    for ( int nCorner = 0; nCorner < 8; nCorner++ )
    {
        double oc[ 3 ];

        for ( int nAxis = 0; nAxis < 3; nAxis++ )
            oc[ nAxis ] = o[ nAxis ] - ( ( nCorner & ( 1 << nAxis ) ) ? m_anInner[ nAxis ] : -m_anInner[ nAxis ] );

        double b = ( oc[ 0 ] * d[ 0 ] ) + ( oc[ 1 ] * d[ 1 ] ) + ( oc[ 2 ] * d[ 2 ] );
        double c = ( oc[ 0 ] * oc[ 0 ] ) + ( oc[ 1 ] * oc[ 1 ] ) + ( oc[ 2 ] * oc[ 2 ] ) - ( r * r );
        double nDet = ( b * b ) - c;

        if ( nDet < 0.0 )
            continue;

        double nRoot = sqrt( nDet );

        Widen( -b - nRoot, -b + nRoot, nEnter, nLeave, bAny );
    }

    if ( ! bAny )
        return FALSE;
    if ( nEnter > nLow && nEnter < nHigh )
    {
        nT = nEnter;
        return TRUE;
    }
    if ( nEnter <= nLow && nLeave > nLow && nLeave < nHigh )
    {
        nT = nLeave;
        return TRUE;
    }
    return FALSE;
}

void CRoundedBox::LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] )
{
        // Away from the nearest point of the inner box.
    FLT nLength = 0.0;

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        FLT nClamped = anPoint[ nAxis ];

        if ( nClamped > m_anInner[ nAxis ] )
            nClamped = m_anInner[ nAxis ];
        if ( nClamped < -m_anInner[ nAxis ] )
            nClamped = -m_anInner[ nAxis ];
        anNormal[ nAxis ] = anPoint[ nAxis ] - nClamped;
        nLength += anNormal[ nAxis ] * anNormal[ nAxis ];
    }
    if ( nLength > 1e-24 * m_nRadius * m_nRadius )
        return;

        // Inside the inner box (not on the surface): the nearest face.
    int nBest = 0;
    FLT nGap = 1e300;

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        FLT nThis = m_anHalf[ nAxis ] - ABS( anPoint[ nAxis ] );

        anNormal[ nAxis ] = 0.0;
        if ( nThis < nGap )
        {
            nGap = nThis;
            nBest = nAxis;
        }
    }
    anNormal[ nBest ] = ( anPoint[ nBest ] >= 0.0 ) ? 1.0 : -1.0;
}

void CRoundedBox::LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV )
{
    FLT anNormal[ 3 ], anLocal[ 3 ];
    int nAxis = 0;

    LocalNormal( anPoint, anNormal );
    for ( int n = 0; n < 3; n++ )
    {
        anLocal[ n ] = anPoint[ n ] / m_anHalf[ n ];
        if ( ABS( anNormal[ n ] ) > ABS( anNormal[ nAxis ] ) )
            nAxis = n;
    }

    int nSign = ( anNormal[ nAxis ] >= 0.0 ) ? 1 : -1;
    FLT nAcross, nUp;

        // As a box's faces: across and up as seen from outside each.
    switch ( nAxis )
    {
        case 0:
            nAcross = anLocal[ 2 ] * nSign;
            nUp = anLocal[ 1 ];
            break;

        case 1:
            nAcross = anLocal[ 0 ];
            nUp = anLocal[ 2 ] * nSign;
            break;

        default:
            nAcross = -anLocal[ 0 ] * nSign;
            nUp = anLocal[ 1 ];
            break;
    }
    nU = 0.5 + ( nAcross / 2.0 );
    nV = 0.5 + ( nUp / 2.0 );
}

void CRoundedBox::LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] )
{
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        anLow[ nAxis ] = -m_anHalf[ nAxis ];
        anHigh[ nAxis ] = m_anHalf[ nAxis ];
    }
}

void CRoundedBox::LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals )
{
        // A sphere of the radius, each point pushed out to its own corner of
        // the inner box (the angles start half a step off the axes, so
        // every point has a corner), which stretches the faces flat.
    int              nRings = ( nSegments / 2 > 6 ) ? nSegments / 2 : 6;
    int              nRow = nSegments + 1;
    std::vector<FLT> anGrid, anGridNormals;

    for ( int nRing = 0; nRing <= nRings; nRing++ )
    {
        FLT nV = ( ( nRing == 0 ) ? 0.0 : ( nRing == nRings ) ? PI : PI * ( nRing - 0.5 ) / ( nRings - 1 ) ) - ( PI / 2.0 );

        for ( int nSegment = 0; nSegment <= nSegments; nSegment++ )
        {
            FLT nU = 2.0 * PI * ( nSegment + 0.5 ) / nSegments;
            FLT anNormal[ 3 ] = { COS( nV ) * COS( nU ), SIN( nV ), COS( nV ) * SIN( nU ) };

            for ( int i = 0; i < 3; i++ )
            {
                FLT nCorner = ( anNormal[ i ] >= 0.0 ) ? m_anInner[ i ] : -m_anInner[ i ];

                anGrid.push_back( nCorner + ( m_nRadius * anNormal[ i ] ) );
                anGridNormals.push_back( anNormal[ i ] );
            }
        }
    }
    for ( int nRing = 0; nRing < nRings; nRing++ )
    {
        for ( int nSegment = 0; nSegment < nSegments; nSegment++ )
        {
            int a = ( ( nRing * nRow ) + nSegment ) * 3, b = a + 3;
            int d = ( ( ( nRing + 1 ) * nRow ) + nSegment ) * 3, c = d + 3;

            LocalMesh::Add( anPositions, anNormals, &anGrid[ a ], &anGrid[ d ], &anGrid[ c ],
                            &anGridNormals[ a ], &anGridNormals[ d ], &anGridNormals[ c ] );
            LocalMesh::Add( anPositions, anNormals, &anGrid[ a ], &anGrid[ c ], &anGrid[ b ],
                            &anGridNormals[ a ], &anGridNormals[ c ], &anGridNormals[ b ] );
        }
    }
}

const char *const *CRoundedBox::ShapeFields( void )
{
    static const char *const astrFields[] = { "size", "radius", NULL };

    return astrFields;
}

void CRoundedBox::ShapeToJson( IASLib::CObjectNode &oNode )
{
    if ( m_oSize.GetX() == m_oSize.GetY() && m_oSize.GetY() == m_oSize.GetZ() )
        RayJson::PutNumber( oNode, "size", m_oSize.GetX() );
    else
        RayJson::PutVector( oNode, "size", m_oSize );
    RayJson::PutNumber( oNode, "radius", m_nRadius );
}

BOOL CRoundedBox::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    const IASLib::CJsonNode *pSize = RayJson::Field( oNode, "size", TRUE );

    if ( pSize == NULL )
        return FALSE;
    if ( pSize->isNumber() )
    {
        FLT nSize = 0.0;

        if ( ! RayJson::GetNumber( oNode, "size", nSize, TRUE ) )
            return FALSE;
        m_oSize.Set( nSize, nSize, nSize );
    }
    else if ( ! RayJson::GetVector( oNode, "size", m_oSize, TRUE ) )
        return FALSE;
    return RayJson::GetNumber( oNode, "radius", m_nRadius, TRUE );
}

void CRoundedBox::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CRoundedBox::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
