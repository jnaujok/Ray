/*************************************************************************
*   CSuperquadric                                                        *
*                                                                        *
*   A superellipsoid. See Superquadric.h.                                *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Superquadric.h"
#include "SceneJson.h"

#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CSuperquadric, "superquadric" );

CSuperquadric::CSuperquadric( void ) :
    m_oRadius( 1.0, 1.0, 1.0 ),
    m_nE1( 1.0 ),
    m_nE2( 1.0 )
{
    Prepare();
}

CSuperquadric::~CSuperquadric( void )
{
}

BOOL CSuperquadric::Set( const CVector &oCenter, const CVector &oRadius, FLT nE1, FLT nE2, const CVector &oRotationDegrees )
{
    m_oCenter = oCenter;
    m_oRadius = oRadius;
    m_nE1 = nE1;
    m_nE2 = nE2;
    m_oRotation = oRotationDegrees;
    return Prepare();
}

BOOL CSuperquadric::PrepareShape( void )
{
    m_anRadius[ 0 ] = m_oRadius.GetX();
    m_anRadius[ 1 ] = m_oRadius.GetY();
    m_anRadius[ 2 ] = m_oRadius.GetZ();
    if ( ! ( m_anRadius[ 0 ] > 0.0 && m_anRadius[ 1 ] > 0.0 && m_anRadius[ 2 ] > 0.0 ) )
        return RayJson::Fail( "every radius must be more than 0" );
    if ( ! ( m_nE1 >= 0.05 && m_nE1 <= 10.0 && m_nE2 >= 0.05 && m_nE2 <= 10.0 ) )
        return RayJson::Fail( "e1 and e2 must be from 0.05 to 10" );
    return TRUE;
}

double CSuperquadric::Field( const double anPoint[ 3 ] ) const
{
    double x = ABS( anPoint[ 0 ] / m_anRadius[ 0 ] );
    double y = ABS( anPoint[ 1 ] / m_anRadius[ 1 ] );
    double z = ABS( anPoint[ 2 ] / m_anRadius[ 2 ] );
    double nAround = pow( x, 2.0 / m_nE2 ) + pow( z, 2.0 / m_nE2 );
    double nF = pow( nAround, m_nE2 / m_nE1 ) + pow( y, 2.0 / m_nE1 );

    return pow( nF, m_nE1 / 2.0 ) - 1.0;
}

double CSuperquadric::FieldAlong( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], double t ) const
{
    double anPoint[ 3 ] = { anOrigin[ 0 ] + ( t * anDirection[ 0 ] ),
                            anOrigin[ 1 ] + ( t * anDirection[ 1 ] ),
                            anOrigin[ 2 ] + ( t * anDirection[ 2 ] ) };

    return Field( anPoint );
}

double CSuperquadric::Bisect( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], double a, double b, double fa ) const
{
    for ( int nStep = 0; nStep < 100; nStep++ )
    {
        double m = ( a + b ) / 2.0;

        if ( m <= a || m >= b )
            break;

        double fm = FieldAlong( anOrigin, anDirection, m );

        if ( ( fm < 0.0 ) == ( fa < 0.0 ) )
        {
            a = m;
            fa = fm;
        }
        else
            b = m;
    }
    return ( a + b ) / 2.0;
}

BOOL CSuperquadric::LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT )
{
        // The stretch of the ray inside the bounding box.
    double nEnter = nLow, nLeave = nHigh;

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        double nHalf = m_anRadius[ nAxis ] * ( 1.0 + 1e-9 );

        if ( anDirection[ nAxis ] == 0.0 )
        {
            if ( ABS( anOrigin[ nAxis ] ) > nHalf )
                return FALSE;
            continue;
        }

        double t1 = ( -nHalf - anOrigin[ nAxis ] ) / anDirection[ nAxis ];
        double t2 = ( nHalf - anOrigin[ nAxis ] ) / anDirection[ nAxis ];

        if ( t1 > t2 )
        {
            double nSwap = t1;

            t1 = t2;
            t2 = nSwap;
        }
        nEnter = ( t1 > nEnter ) ? t1 : nEnter;
        nLeave = ( t2 < nLeave ) ? t2 : nLeave;
    }
    if ( ! ( nLeave > nEnter ) )
        return FALSE;

    double nStart = FieldAlong( anOrigin, anDirection, nEnter );
    double nRoot;

    if ( IsConvex() )
    {
        if ( nStart < 0.0 )
        {
                // From inside: the one place it leaves.
            if ( FieldAlong( anOrigin, anDirection, nLeave ) < 0.0 )
                return FALSE;
            nRoot = Bisect( anOrigin, anDirection, nEnter, nLeave, nStart );
        }
        else
        {
                // The lowest point along the ray: if it is inside, the ray
                // enters between the box and there.
            const double nGolden = 0.6180339887498949;
            double       a = nEnter, b = nLeave;
            double       c = b - ( nGolden * ( b - a ) ), d = a + ( nGolden * ( b - a ) );
            double       fc = FieldAlong( anOrigin, anDirection, c ), fd = FieldAlong( anOrigin, anDirection, d );

            for ( int nStep = 0; nStep < 80 && fc >= 0.0 && fd >= 0.0; nStep++ )
            {
                if ( fc < fd )
                {
                    b = d;
                    d = c;
                    fd = fc;
                    c = b - ( nGolden * ( b - a ) );
                    fc = FieldAlong( anOrigin, anDirection, c );
                }
                else
                {
                    a = c;
                    c = d;
                    fc = fd;
                    d = a + ( nGolden * ( b - a ) );
                    fd = FieldAlong( anOrigin, anDirection, d );
                }
            }

            double nInside = ( fc < fd ) ? c : d;

            if ( FieldAlong( anOrigin, anDirection, nInside ) > 0.0 )
                return FALSE;
            nRoot = Bisect( anOrigin, anDirection, nEnter, nInside, nStart );
        }
    }
    else
    {
            // Pinched shapes: step through the box for the first change.
        const int nSteps = 256;
        double    a = nEnter, fa = nStart;
        BOOL      bFound = FALSE;

        for ( int nStep = 1; nStep <= nSteps; nStep++ )
        {
            double b = nEnter + ( ( nLeave - nEnter ) * nStep / nSteps );
            double fb = FieldAlong( anOrigin, anDirection, b );

            if ( ( fb < 0.0 ) != ( fa < 0.0 ) )
            {
                nRoot = Bisect( anOrigin, anDirection, a, b, fa );
                bFound = TRUE;
                break;
            }
            a = b;
            fa = fb;
        }
        if ( ! bFound )
            return FALSE;
    }
    if ( ! ( nRoot > nLow && nRoot < nHigh ) )
        return FALSE;
    nT = nRoot;
    return TRUE;
}

void CSuperquadric::LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] )
{
        // The field's gradient, by central differences.
    double nStep = 1e-6 * ( m_anRadius[ 0 ] + m_anRadius[ 1 ] + m_anRadius[ 2 ] );

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        double anAbove[ 3 ] = { anPoint[ 0 ], anPoint[ 1 ], anPoint[ 2 ] };
        double anBelow[ 3 ] = { anPoint[ 0 ], anPoint[ 1 ], anPoint[ 2 ] };

        anAbove[ nAxis ] += nStep;
        anBelow[ nAxis ] -= nStep;
        anNormal[ nAxis ] = (FLT)( Field( anAbove ) - Field( anBelow ) );
    }
    if ( anNormal[ 0 ] == 0.0 && anNormal[ 1 ] == 0.0 && anNormal[ 2 ] == 0.0 )
        anNormal[ 1 ] = 1.0;
}

void CSuperquadric::LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV )
{
    FLT x = anPoint[ 0 ] / m_anRadius[ 0 ], y = anPoint[ 1 ] / m_anRadius[ 1 ], z = anPoint[ 2 ] / m_anRadius[ 2 ];

    nU = 0.5 + ( (FLT)atan2( (double)z, (double)x ) / ( 2.0 * PI ) );
    nV = 0.5 + ( (FLT)atan2( (double)y, sqrt( (double)( ( x * x ) + ( z * z ) ) ) ) / PI );
}

void CSuperquadric::LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] )
{
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        anLow[ nAxis ] = -m_anRadius[ nAxis ];
        anHigh[ nAxis ] = m_anRadius[ nAxis ];
    }
}

    // |base|^power with base's sign.
static FLT SignedPower( FLT nBase, FLT nPower )
{
    FLT nValue = (FLT)pow( (double)ABS( nBase ), (double)nPower );

    return ( nBase < 0.0 ) ? -nValue : nValue;
}

void CSuperquadric::LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals )
{
    int nRings = ( nSegments / 2 > 6 ) ? nSegments / 2 : 6;
    std::vector<FLT> anGrid, anGridNormals;

    for ( int nRing = 0; nRing <= nRings; nRing++ )
    {
        FLT nV = ( PI * nRing / nRings ) - ( PI / 2.0 );

        for ( int nSegment = 0; nSegment <= nSegments; nSegment++ )
        {
            FLT nU = ( 2.0 * PI * nSegment / nSegments ) - PI;
            FLT anPoint[ 3 ], anNormal[ 3 ];

            anPoint[ 0 ] = m_anRadius[ 0 ] * SignedPower( COS( nV ), m_nE1 ) * SignedPower( COS( nU ), m_nE2 );
            anPoint[ 1 ] = m_anRadius[ 1 ] * SignedPower( SIN( nV ), m_nE1 );
            anPoint[ 2 ] = m_anRadius[ 2 ] * SignedPower( COS( nV ), m_nE1 ) * SignedPower( SIN( nU ), m_nE2 );
            LocalNormal( anPoint, anNormal );

            FLT nLength = SQRT( ( anNormal[ 0 ] * anNormal[ 0 ] ) + ( anNormal[ 1 ] * anNormal[ 1 ] ) + ( anNormal[ 2 ] * anNormal[ 2 ] ) );

            for ( int i = 0; i < 3; i++ )
            {
                anGrid.push_back( anPoint[ i ] );
                anGridNormals.push_back( anNormal[ i ] / nLength );
            }
        }
    }

    int nRow = nSegments + 1;

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

const char *const *CSuperquadric::ShapeFields( void )
{
    static const char *const astrFields[] = { "radius", "e1", "e2", NULL };

    return astrFields;
}

void CSuperquadric::ShapeToJson( IASLib::CObjectNode &oNode )
{
    if ( m_oRadius.GetX() == m_oRadius.GetY() && m_oRadius.GetY() == m_oRadius.GetZ() )
        RayJson::PutNumber( oNode, "radius", m_oRadius.GetX() );
    else
        RayJson::PutVector( oNode, "radius", m_oRadius );
    RayJson::PutNumber( oNode, "e1", m_nE1 );
    RayJson::PutNumber( oNode, "e2", m_nE2 );
}

BOOL CSuperquadric::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    const IASLib::CJsonNode *pRadius = RayJson::Field( oNode, "radius", TRUE );

    m_nE1 = m_nE2 = 1.0;
    if ( pRadius == NULL )
        return FALSE;
    if ( pRadius->isNumber() )
    {
        FLT nRadius = 0.0;

        if ( ! RayJson::GetNumber( oNode, "radius", nRadius, TRUE ) )
            return FALSE;
        m_oRadius.Set( nRadius, nRadius, nRadius );
    }
    else if ( ! RayJson::GetVector( oNode, "radius", m_oRadius, TRUE ) )
        return FALSE;
    return RayJson::GetNumber( oNode, "e1", m_nE1, FALSE ) &&
           RayJson::GetNumber( oNode, "e2", m_nE2, FALSE );
}

void CSuperquadric::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CSuperquadric::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
