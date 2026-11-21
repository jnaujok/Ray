/*************************************************************************
*   CLocalShape                                                          *
*                                                                        *
*   The base of the shapes worked out in a frame of their own. See       *
*   LocalShape.h.                                                        *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "LocalShape.h"
#include "SceneJson.h"

#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CLocalShape::CLocalShape( void ) :
    m_oCenter( 0.0, 0.0, 0.0 ),
    m_oRotation( 0.0, 0.0, 0.0 ),
    m_nMinimumT( 1e-9 )
{
    m_oMaterials.Add( new CDiffuseMaterial, 1.0 );
    m_oColor.Set( 0.5, 0.5, 0.5 );
    m_aoAxes[ 0 ].Set( 1.0, 0.0, 0.0 );
    m_aoAxes[ 1 ].Set( 0.0, 1.0, 0.0 );
    m_aoAxes[ 2 ].Set( 0.0, 0.0, 1.0 );
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        for ( int i = 0; i < 3; i++ )
            m_anAxes[ nAxis ][ i ] = ( nAxis == i ) ? 1.0 : 0.0;
    }
}

CLocalShape::~CLocalShape( void )
{
}

#ifdef RAY_GUI
void CLocalShape::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hShape;

    hShape = pTree->AddObject( this, 8, "Shape", hUnder );
    CObjects::MatToTree( pTree, hShape );
}
#endif

void CLocalShape::RotateXYZ( FLT anVector[ 3 ], FLT nX, FLT nY, FLT nZ )
{
    FLT nA, nB;

    nA = ( anVector[ 1 ] * COS( nX ) ) - ( anVector[ 2 ] * SIN( nX ) );
    nB = ( anVector[ 1 ] * SIN( nX ) ) + ( anVector[ 2 ] * COS( nX ) );
    anVector[ 1 ] = nA;
    anVector[ 2 ] = nB;

    nA = ( anVector[ 0 ] * COS( nY ) ) + ( anVector[ 2 ] * SIN( nY ) );
    nB = ( anVector[ 2 ] * COS( nY ) ) - ( anVector[ 0 ] * SIN( nY ) );
    anVector[ 0 ] = nA;
    anVector[ 2 ] = nB;

    nA = ( anVector[ 0 ] * COS( nZ ) ) - ( anVector[ 1 ] * SIN( nZ ) );
    nB = ( anVector[ 0 ] * SIN( nZ ) ) + ( anVector[ 1 ] * COS( nZ ) );
    anVector[ 0 ] = nA;
    anVector[ 1 ] = nB;
}

BOOL CLocalShape::Prepare( void )
{
    FLT nToRadians = PI / 180.0;

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        FLT anAxis[ 3 ] = { 0.0, 0.0, 0.0 };

        anAxis[ nAxis ] = 1.0;
        RotateXYZ( anAxis, m_oRotation.GetX() * nToRadians, m_oRotation.GetY() * nToRadians, m_oRotation.GetZ() * nToRadians );
        m_aoAxes[ nAxis ].Set( anAxis[ 0 ], anAxis[ 1 ], anAxis[ 2 ] );
        for ( int i = 0; i < 3; i++ )
            m_anAxes[ nAxis ][ i ] = anAxis[ i ];
    }
    if ( ! PrepareShape() )
        return FALSE;

        // Hits closer than this to where a ray starts are the surface it
        // leaves from, found again through rounding.
    FLT anLow[ 3 ], anHigh[ 3 ];

    LocalBounds( anLow, anHigh );
    m_nMinimumT = 1e-7 * ( ( anHigh[ 0 ] - anLow[ 0 ] ) + ( anHigh[ 1 ] - anLow[ 1 ] ) + ( anHigh[ 2 ] - anLow[ 2 ] ) );
    return TRUE;
}

void CLocalShape::ToLocal( CVector &oPoint, FLT anLocal[ 3 ] )
{
    FLT anOffset[ 3 ] = { oPoint.GetX() - m_oCenter.GetX(), oPoint.GetY() - m_oCenter.GetY(), oPoint.GetZ() - m_oCenter.GetZ() };

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
        anLocal[ nAxis ] = ( anOffset[ 0 ] * m_anAxes[ nAxis ][ 0 ] ) + ( anOffset[ 1 ] * m_anAxes[ nAxis ][ 1 ] ) +
                           ( anOffset[ 2 ] * m_anAxes[ nAxis ][ 2 ] );
}

void CLocalShape::DirectionToLocal( CVector &oDirection, FLT anLocal[ 3 ] )
{
    FLT anIn[ 3 ] = { oDirection.GetX(), oDirection.GetY(), oDirection.GetZ() };

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
        anLocal[ nAxis ] = ( anIn[ 0 ] * m_anAxes[ nAxis ][ 0 ] ) + ( anIn[ 1 ] * m_anAxes[ nAxis ][ 1 ] ) + ( anIn[ 2 ] * m_anAxes[ nAxis ][ 2 ] );
}

CVector CLocalShape::FromLocalDirection( const FLT anLocal[ 3 ] )
{
    FLT anOut[ 3 ];

    for ( int i = 0; i < 3; i++ )
        anOut[ i ] = ( m_anAxes[ 0 ][ i ] * anLocal[ 0 ] ) + ( m_anAxes[ 1 ][ i ] * anLocal[ 1 ] ) + ( m_anAxes[ 2 ][ i ] * anLocal[ 2 ] );
    return CVector( anOut[ 0 ], anOut[ 1 ], anOut[ 2 ] );
}

CVector CLocalShape::FromLocalPoint( const FLT anLocal[ 3 ] )
{
    CVector oOffset = FromLocalDirection( anLocal );

    return CVector( m_oCenter.GetX() + oOffset.GetX(), m_oCenter.GetY() + oOffset.GetY(), m_oCenter.GetZ() + oOffset.GetZ() );
}

BOOL CLocalShape::Hit( CRay &oRay, FLT nMinimumT, FLT &nT )
{
    CVector oOrigin( oRay.GetX(), oRay.GetY(), oRay.GetZ() );
    CVector oDirection( oRay.GetI(), oRay.GetJ(), oRay.GetK() );
    FLT     nLength = oDirection.Magnitude();
    FLT     anOrigin[ 3 ], anDirection[ 3 ];

    if ( ! ( nLength > 0.0 ) )
        return FALSE;
    ToLocal( oOrigin, anOrigin );
    DirectionToLocal( oDirection, anDirection );
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
        anDirection[ nAxis ] /= nLength;

        // The subclass works with a unit direction: distances along it are
        // the ray's t times its length.
    FLT nUnitT;

    if ( ! LocalHit( anOrigin, anDirection, nMinimumT * nLength, oRay.GetT() * nLength, nUnitT ) )
        return FALSE;
    nT = nUnitT / nLength;
    return ( nT > nMinimumT && nT < oRay.GetT() ) ? TRUE : FALSE;
}

BOOL CLocalShape::Intersect( CRay &oRay, CVector &oIntersect )
{
    FLT nT;

    if ( ! Hit( oRay, 0.0, nT ) )
        return FALSE;

    oRay.SetT( nT );
    oIntersect.Set( ( oRay.GetI() * nT ) + oRay.GetX(),
                    ( oRay.GetJ() * nT ) + oRay.GetY(),
                    ( oRay.GetK() * nT ) + oRay.GetZ() );
    return TRUE;
}

BOOL CLocalShape::Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore )
{
    if ( pIgnore != this )
        return Intersect( oRay, oIntersect );
    if ( IsClosed() && IsConvex() )
        return FALSE;

        // A shape that can meet a ray again after it leaves the surface (a
        // torus, an open shell) is tested past the point the ray leaves.
    FLT nT;

    if ( ! Hit( oRay, m_nMinimumT, nT ) )
        return FALSE;

    oRay.SetT( nT );
    oIntersect.Set( ( oRay.GetI() * nT ) + oRay.GetX(),
                    ( oRay.GetJ() * nT ) + oRay.GetY(),
                    ( oRay.GetK() * nT ) + oRay.GetZ() );
    return TRUE;
}

CVector CLocalShape::NormalAt( CVector &oPoint )
{
    FLT anPoint[ 3 ], anNormal[ 3 ];

    ToLocal( oPoint, anPoint );
    LocalNormal( anPoint, anNormal );

    CVector oNormal = FromLocalDirection( anNormal );

    oNormal.Normalize();
    return oNormal;
}

CColor CLocalShape::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
    CVector oNormal = NormalAt( oIntersect );
    CVector oDirection( oRay.GetI(), oRay.GetJ(), oRay.GetK() );

        // An open surface (a disc, an uncapped paraboloid) shows the side
        // the ray sees.
    if ( ! IsClosed() && oNormal.Dot( oDirection ) > 0.0 )
        oNormal = oNormal * -1.0;

    return BaseShade( oScene, oRay, oIntersect, oNormal );
}

BOOL CLocalShape::SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV )
{
    FLT anPoint[ 3 ];

    ToLocal( oPoint, anPoint );
    LocalCoords( anPoint, nU, nV );
    return TRUE;
}

    // A box a little larger than [anLow, anHigh], so hits a hair outside the
    // exact shape are inside it.
static void PadBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ], CVector &oMinimum, CVector &oMaximum )
{
    FLT nMargin = 1e-6 * ( 1.0 + ( anHigh[ 0 ] - anLow[ 0 ] ) + ( anHigh[ 1 ] - anLow[ 1 ] ) + ( anHigh[ 2 ] - anLow[ 2 ] ) );

    oMinimum.Set( anLow[ 0 ] - nMargin, anLow[ 1 ] - nMargin, anLow[ 2 ] - nMargin );
    oMaximum.Set( anHigh[ 0 ] + nMargin, anHigh[ 1 ] + nMargin, anHigh[ 2 ] + nMargin );
}

BOOL CLocalShape::GetBounds( CVector &oMinimum, CVector &oMaximum )
{
    FLT anLow[ 3 ], anHigh[ 3 ];
    FLT anCenter[ 3 ] = { m_oCenter.GetX(), m_oCenter.GetY(), m_oCenter.GetZ() };
    FLT anSceneLow[ 3 ], anSceneHigh[ 3 ];

    LocalBounds( anLow, anHigh );

        // The turned box's reach along each scene axis, about its own center.
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        FLT nMiddle = anCenter[ nAxis ];
        FLT nReach = 0.0;

        for ( int nOwn = 0; nOwn < 3; nOwn++ )
        {
            CVector &oOwn = m_aoAxes[ nOwn ];
            FLT      nAlong = ( nAxis == 0 ) ? oOwn.GetX() : ( nAxis == 1 ) ? oOwn.GetY() : oOwn.GetZ();

            nMiddle += nAlong * ( anLow[ nOwn ] + anHigh[ nOwn ] ) / 2.0;
            nReach += ABS( nAlong ) * ( anHigh[ nOwn ] - anLow[ nOwn ] ) / 2.0;
        }
        anSceneLow[ nAxis ] = nMiddle - nReach;
        anSceneHigh[ nAxis ] = nMiddle + nReach;
    }
    PadBounds( anSceneLow, anSceneHigh, oMinimum, oMaximum );
    return TRUE;
}

void CLocalShape::Triangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals )
{
    std::vector<FLT> anLocal, anLocalNormals;

    LocalTriangles( nSegments, anLocal, anLocalNormals );
    anPositions.clear();
    anNormals.clear();
    for ( size_t n = 0; n + 2 < anLocal.size(); n += 3 )
    {
        CVector oPoint = FromLocalPoint( &anLocal[ n ] );
        CVector oNormal = FromLocalDirection( &anLocalNormals[ n ] );

        anPositions.push_back( oPoint.GetX() );
        anPositions.push_back( oPoint.GetY() );
        anPositions.push_back( oPoint.GetZ() );
        anNormals.push_back( oNormal.GetX() );
        anNormals.push_back( oNormal.GetY() );
        anNormals.push_back( oNormal.GetZ() );
    }
}

void CLocalShape::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "center", m_oCenter );
    ShapeToJson( oNode );
    if ( m_oRotation.GetX() != 0.0 || m_oRotation.GetY() != 0.0 || m_oRotation.GetZ() != 0.0 )
        RayJson::PutVector( oNode, "rotation", m_oRotation );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
    BumpToJson( oNode );
}

BOOL CLocalShape::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrCommon[] = { "center", "rotation", "color", "materials", "bump" };
    std::vector<const char *> astrFields( astrCommon, astrCommon + 5 );

    for ( const char *const *pstrOwn = ShapeFields(); *pstrOwn; pstrOwn++ )
        astrFields.push_back( *pstrOwn );
    astrFields.push_back( NULL );

    return RayJson::OnlyFields( oNode, &astrFields[ 0 ] ) &&
           RayJson::GetVector( oNode, "center", m_oCenter, TRUE ) &&
           RayJson::GetVector( oNode, "rotation", m_oRotation, FALSE ) &&
           RayJson::GetColor( oNode, "color", m_oColor, FALSE ) &&
           ShapeFromJson( oNode ) &&
           m_oMaterials.FromJson( oNode, "materials" ) &&
           BumpFromJson( oNode ) &&
           Prepare();
}

    // The polynomial at x, by Horner's rule.
static double Evaluate( const double *anCoeffs, int nDegree, double x )
{
    double nValue = anCoeffs[ nDegree ];

    for ( int n = nDegree - 1; n >= 0; n-- )
        nValue = ( nValue * x ) + anCoeffs[ n ];
    return nValue;
}

int CLocalShape::PolynomialRoots( const double *anCoeffs, int nDegree, double nLow, double nHigh, double *anRoots )
{
        // Leading coefficients that are zero (to rounding) lower the degree.
    double nScale = 0.0;

    for ( int n = 0; n <= nDegree; n++ )
        nScale = ( ABS( anCoeffs[ n ] ) > nScale ) ? ABS( anCoeffs[ n ] ) : nScale;
    if ( nScale == 0.0 )
        return 0;
    while ( nDegree > 0 && ABS( anCoeffs[ nDegree ] ) <= 1e-15 * nScale )
        nDegree--;
    if ( nDegree < 1 || ! ( nHigh > nLow ) )
        return 0;
    if ( nDegree == 1 )
    {
        double x = -anCoeffs[ 0 ] / anCoeffs[ 1 ];

        if ( x >= nLow && x <= nHigh )
        {
            anRoots[ 0 ] = x;
            return 1;
        }
        return 0;
    }

        // The turning points split [nLow, nHigh] into stretches where the
        // polynomial only rises or only falls, so each holds one root at
        // most, found where its sign changes.
    double anDerivative[ 8 ];
    double anTurns[ 8 ];
    double anEnds[ 10 ];
    int    nTurns, nEnds = 0, nRoots = 0;

    for ( int n = 1; n <= nDegree; n++ )
        anDerivative[ n - 1 ] = anCoeffs[ n ] * n;
    nTurns = PolynomialRoots( anDerivative, nDegree - 1, nLow, nHigh, anTurns );

    anEnds[ nEnds++ ] = nLow;
    for ( int n = 0; n < nTurns; n++ )
        anEnds[ nEnds++ ] = anTurns[ n ];
    anEnds[ nEnds++ ] = nHigh;

    for ( int n = 0; n + 1 < nEnds; n++ )
    {
        double a = anEnds[ n ], b = anEnds[ n + 1 ];
        double fa = Evaluate( anCoeffs, nDegree, a );
        double fb = Evaluate( anCoeffs, nDegree, b );

        if ( fa == 0.0 )
        {
            if ( nRoots == 0 || anRoots[ nRoots - 1 ] != a )
                anRoots[ nRoots++ ] = a;
            continue;
        }
        if ( ( fa < 0.0 ) == ( fb < 0.0 ) || fb == 0.0 )
        {
            if ( fb == 0.0 && n + 2 == nEnds )
                anRoots[ nRoots++ ] = b;
            continue;
        }
        for ( int nStep = 0; nStep < 200; nStep++ )
        {
            double m = ( a + b ) / 2.0;

            if ( m <= a || m >= b )
                break;

            double fm = Evaluate( anCoeffs, nDegree, m );

            if ( fm == 0.0 )
            {
                a = b = m;
                break;
            }
            if ( ( fm < 0.0 ) == ( fa < 0.0 ) )
            {
                a = m;
                fa = fm;
            }
            else
                b = m;
        }
        anRoots[ nRoots++ ] = ( a + b ) / 2.0;
    }
    return nRoots;
}

namespace LocalMesh
{
    void Add( std::vector<FLT> &anPositions, std::vector<FLT> &anNormals, const FLT a[ 3 ], const FLT b[ 3 ], const FLT c[ 3 ],
              const FLT na[ 3 ], const FLT nb[ 3 ], const FLT nc[ 3 ] )
    {
        const FLT *apPoints[ 3 ] = { a, b, c };
        const FLT *apNormals[ 3 ] = { na, nb, nc };

        for ( int k = 0; k < 3; k++ )
        {
            for ( int i = 0; i < 3; i++ )
            {
                anPositions.push_back( apPoints[ k ][ i ] );
                anNormals.push_back( apNormals[ k ][ i ] );
            }
        }
    }

    void Revolve( std::vector<FLT> &anPositions, std::vector<FLT> &anNormals, const std::vector<FLT> &anProfile, int nSegments )
    {
        size_t nPoints = anProfile.size() / 4;

        for ( int nSegment = 0; nSegment < nSegments; nSegment++ )
        {
            FLT nA0 = 2.0 * PI * nSegment / nSegments, nA1 = 2.0 * PI * ( nSegment + 1 ) / nSegments;
            FLT c0 = COS( nA0 ), s0 = SIN( nA0 ), c1 = COS( nA1 ), s1 = SIN( nA1 );

            for ( size_t n = 0; n + 1 < nPoints; n++ )
            {
                const FLT *p = &anProfile[ n * 4 ];
                const FLT *q = &anProfile[ ( n + 1 ) * 4 ];
                FLT a[ 3 ] = { p[ 0 ] * c0, p[ 1 ], p[ 0 ] * s0 }, na[ 3 ] = { p[ 2 ] * c0, p[ 3 ], p[ 2 ] * s0 };
                FLT b[ 3 ] = { p[ 0 ] * c1, p[ 1 ], p[ 0 ] * s1 }, nb[ 3 ] = { p[ 2 ] * c1, p[ 3 ], p[ 2 ] * s1 };
                FLT c[ 3 ] = { q[ 0 ] * c1, q[ 1 ], q[ 0 ] * s1 }, nc[ 3 ] = { q[ 2 ] * c1, q[ 3 ], q[ 2 ] * s1 };
                FLT d[ 3 ] = { q[ 0 ] * c0, q[ 1 ], q[ 0 ] * s0 }, nd[ 3 ] = { q[ 2 ] * c0, q[ 3 ], q[ 2 ] * s0 };

                Add( anPositions, anNormals, a, b, c, na, nb, nc );
                Add( anPositions, anNormals, a, c, d, na, nc, nd );
            }
        }
    }

    void Ring( std::vector<FLT> &anPositions, std::vector<FLT> &anNormals, FLT nY, FLT nInner, FLT nOuter, BOOL bUp, int nSegments )
    {
        FLT anUp[ 3 ] = { 0.0, bUp ? 1.0 : -1.0, 0.0 };

        for ( int nSegment = 0; nSegment < nSegments; nSegment++ )
        {
            FLT nA0 = 2.0 * PI * nSegment / nSegments, nA1 = 2.0 * PI * ( nSegment + 1 ) / nSegments;
            FLT c0 = COS( nA0 ), s0 = SIN( nA0 ), c1 = COS( nA1 ), s1 = SIN( nA1 );
            FLT a[ 3 ] = { nInner * c0, nY, nInner * s0 }, b[ 3 ] = { nOuter * c0, nY, nOuter * s0 };
            FLT c[ 3 ] = { nOuter * c1, nY, nOuter * s1 }, d[ 3 ] = { nInner * c1, nY, nInner * s1 };

            Add( anPositions, anNormals, a, b, c, anUp, anUp, anUp );
            if ( nInner > 0.0 )
                Add( anPositions, anNormals, a, c, d, anUp, anUp, anUp );
        }
    }
}
