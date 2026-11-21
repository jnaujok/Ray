/*************************************************************************
*   CTorus                                                               *
*                                                                        *
*   A ring. See Torus.h.                                                 *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Torus.h"
#include "SceneJson.h"

#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CTorus, "torus" );

CTorus::CTorus( void ) :
    m_nMajor( 1.0 ),
    m_nMinor( 0.25 )
{
    Prepare();
}

CTorus::~CTorus( void )
{
}

BOOL CTorus::Set( const CVector &oCenter, FLT nMajor, FLT nMinor, const CVector &oRotationDegrees )
{
    m_oCenter = oCenter;
    m_nMajor = nMajor;
    m_nMinor = nMinor;
    m_oRotation = oRotationDegrees;
    return Prepare();
}

BOOL CTorus::PrepareShape( void )
{
    if ( ! ( m_nMinor > 0.0 ) )
        return RayJson::Fail( "minorRadius must be more than 0" );
    if ( ! ( m_nMajor > 0.0 ) )
        return RayJson::Fail( "majorRadius must be more than 0" );
    return TRUE;
}

BOOL CTorus::LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT )
{
        // Where the ray is inside the bounding sphere.
    double nOuter = m_nMajor + m_nMinor;
    double b = ( anOrigin[ 0 ] * anDirection[ 0 ] ) + ( anOrigin[ 1 ] * anDirection[ 1 ] ) + ( anOrigin[ 2 ] * anDirection[ 2 ] );
    double c = ( anOrigin[ 0 ] * anOrigin[ 0 ] ) + ( anOrigin[ 1 ] * anOrigin[ 1 ] ) + ( anOrigin[ 2 ] * anOrigin[ 2 ] ) - ( nOuter * nOuter );
    double nDet = ( b * b ) - c;

    if ( nDet < 0.0 )
        return FALSE;

    double nRoot = sqrt( nDet );
    double nEnter = -b - nRoot, nLeave = -b + nRoot;

    if ( nLeave <= nLow || nEnter >= nHigh )
        return FALSE;

        // The quartic in s, measured from where the ray enters the sphere
        // (or from where it starts, inside it), so its numbers stay small.
    double nStart = ( nEnter > 0.0 ) ? nEnter : 0.0;
    double o[ 3 ] = { anOrigin[ 0 ] + ( nStart * anDirection[ 0 ] ),
                      anOrigin[ 1 ] + ( nStart * anDirection[ 1 ] ),
                      anOrigin[ 2 ] + ( nStart * anDirection[ 2 ] ) };
    const FLT *d = anDirection;
    double R2 = m_nMajor * m_nMajor;
    double nB = ( o[ 0 ] * d[ 0 ] ) + ( o[ 1 ] * d[ 1 ] ) + ( o[ 2 ] * d[ 2 ] );
    double nK = ( o[ 0 ] * o[ 0 ] ) + ( o[ 1 ] * o[ 1 ] ) + ( o[ 2 ] * o[ 2 ] ) + R2 - ( m_nMinor * m_nMinor );
    double nAcross = ( d[ 0 ] * d[ 0 ] ) + ( d[ 2 ] * d[ 2 ] );
    double nOD = ( o[ 0 ] * d[ 0 ] ) + ( o[ 2 ] * d[ 2 ] );
    double nOO = ( o[ 0 ] * o[ 0 ] ) + ( o[ 2 ] * o[ 2 ] );
    double anCoeffs[ 5 ];

        // ( |P|^2 + R^2 - r^2 )^2 = 4 R^2 ( Px^2 + Pz^2 ), P = o + s d.
    anCoeffs[ 4 ] = 1.0;
    anCoeffs[ 3 ] = 4.0 * nB;
    anCoeffs[ 2 ] = ( 4.0 * nB * nB ) + ( 2.0 * nK ) - ( 4.0 * R2 * nAcross );
    anCoeffs[ 1 ] = ( 4.0 * nB * nK ) - ( 8.0 * R2 * nOD );
    anCoeffs[ 0 ] = ( nK * nK ) - ( 4.0 * R2 * nOO );

    double nFrom = ( nLow - nStart > 0.0 ) ? nLow - nStart : 0.0;
    double nTo = ( ( nHigh < nLeave ) ? nHigh : nLeave ) - nStart;
    double anRoots[ 4 ];
    int    nRoots = PolynomialRoots( anCoeffs, 4, nFrom, nTo, anRoots );

    for ( int n = 0; n < nRoots; n++ )
    {
        double t = anRoots[ n ] + nStart;

        if ( t > nLow && t < nHigh )
        {
            nT = t;
            return TRUE;
        }
    }
    return FALSE;
}

void CTorus::LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] )
{
    FLT nRadial = SQRT( ( anPoint[ 0 ] * anPoint[ 0 ] ) + ( anPoint[ 2 ] * anPoint[ 2 ] ) );

        // Away from the nearest point on the circle at the tube's heart.
    if ( nRadial < 1e-300 )
    {
        anNormal[ 0 ] = 0.0;
        anNormal[ 1 ] = ( anPoint[ 1 ] >= 0.0 ) ? 1.0 : -1.0;
        anNormal[ 2 ] = 0.0;
        return;
    }
    anNormal[ 0 ] = anPoint[ 0 ] - ( m_nMajor * anPoint[ 0 ] / nRadial );
    anNormal[ 1 ] = anPoint[ 1 ];
    anNormal[ 2 ] = anPoint[ 2 ] - ( m_nMajor * anPoint[ 2 ] / nRadial );
}

void CTorus::LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV )
{
    FLT nRadial = SQRT( ( anPoint[ 0 ] * anPoint[ 0 ] ) + ( anPoint[ 2 ] * anPoint[ 2 ] ) );
    FLT nTube = (FLT)atan2( (double)anPoint[ 1 ], (double)( nRadial - m_nMajor ) );

    nU = 0.5 + ( (FLT)atan2( (double)anPoint[ 2 ], (double)anPoint[ 0 ] ) / ( 2.0 * PI ) );
    nV = ( nTube >= 0.0 ) ? nTube / ( 2.0 * PI ) : 1.0 + ( nTube / ( 2.0 * PI ) );
}

void CTorus::LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] )
{
    FLT nOuter = m_nMajor + m_nMinor;

    anLow[ 0 ] = -nOuter;
    anLow[ 1 ] = -m_nMinor;
    anLow[ 2 ] = -nOuter;
    anHigh[ 0 ] = nOuter;
    anHigh[ 1 ] = m_nMinor;
    anHigh[ 2 ] = nOuter;
}

void CTorus::LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals )
{
    std::vector<FLT> anProfile;
    int              nAround = ( nSegments / 2 > 8 ) ? nSegments / 2 : 8;

    for ( int n = 0; n <= nAround; n++ )
    {
        FLT nAngle = 2.0 * PI * n / nAround;

        anProfile.push_back( m_nMajor + ( m_nMinor * COS( nAngle ) ) );
        anProfile.push_back( m_nMinor * SIN( nAngle ) );
        anProfile.push_back( COS( nAngle ) );
        anProfile.push_back( SIN( nAngle ) );
    }
    LocalMesh::Revolve( anPositions, anNormals, anProfile, nSegments );
}

const char *const *CTorus::ShapeFields( void )
{
    static const char *const astrFields[] = { "majorRadius", "minorRadius", NULL };

    return astrFields;
}

void CTorus::ShapeToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutNumber( oNode, "majorRadius", m_nMajor );
    RayJson::PutNumber( oNode, "minorRadius", m_nMinor );
}

BOOL CTorus::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    return RayJson::GetNumber( oNode, "majorRadius", m_nMajor, TRUE ) &&
           RayJson::GetNumber( oNode, "minorRadius", m_nMinor, TRUE );
}

void CTorus::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CTorus::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
