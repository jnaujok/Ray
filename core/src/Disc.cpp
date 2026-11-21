/*************************************************************************
*   CDisc                                                                *
*                                                                        *
*   A flat disc, or a ring. See Disc.h.                                  *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Disc.h"
#include "SceneJson.h"

#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DYNAMIC_IMPLEMENTS( CDisc, "disc" );

CDisc::CDisc( void ) :
    m_nRadius( 1.0 ),
    m_nInner( 0.0 )
{
    Prepare();
}

CDisc::~CDisc( void )
{
}

BOOL CDisc::Set( const CVector &oCenter, FLT nRadius, FLT nInner, const CVector &oRotationDegrees )
{
    m_oCenter = oCenter;
    m_nRadius = nRadius;
    m_nInner = nInner;
    m_oRotation = oRotationDegrees;
    return Prepare();
}

BOOL CDisc::PrepareShape( void )
{
    if ( ! ( m_nRadius > 0.0 ) )
        return RayJson::Fail( "the radius must be more than 0" );
    if ( m_nInner < 0.0 || ! ( m_nInner < m_nRadius ) )
        return RayJson::Fail( "innerRadius must be at least 0 and less than the radius" );
    return TRUE;
}

BOOL CDisc::LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT )
{
    if ( anDirection[ 1 ] == 0.0 )
        return FALSE;

    FLT t = -anOrigin[ 1 ] / anDirection[ 1 ];
    FLT x = anOrigin[ 0 ] + ( t * anDirection[ 0 ] );
    FLT z = anOrigin[ 2 ] + ( t * anDirection[ 2 ] );
    FLT r2 = ( x * x ) + ( z * z );

    if ( t <= nLow || t >= nHigh || r2 > m_nRadius * m_nRadius || r2 < m_nInner * m_nInner )
        return FALSE;
    nT = t;
    return TRUE;
}

void CDisc::LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] )
{
    (void)anPoint;
    anNormal[ 0 ] = 0.0;
    anNormal[ 1 ] = 1.0;
    anNormal[ 2 ] = 0.0;
}

void CDisc::LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV )
{
    FLT nRadial = SQRT( ( anPoint[ 0 ] * anPoint[ 0 ] ) + ( anPoint[ 2 ] * anPoint[ 2 ] ) );

    nU = 0.5 + ( (FLT)atan2( (double)anPoint[ 2 ], (double)anPoint[ 0 ] ) / ( 2.0 * PI ) );
    nV = ( nRadial - m_nInner ) / ( m_nRadius - m_nInner );
}

void CDisc::LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] )
{
    anLow[ 0 ] = -m_nRadius;
    anLow[ 1 ] = 0.0;
    anLow[ 2 ] = -m_nRadius;
    anHigh[ 0 ] = m_nRadius;
    anHigh[ 1 ] = 0.0;
    anHigh[ 2 ] = m_nRadius;
}

void CDisc::LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals )
{
    LocalMesh::Ring( anPositions, anNormals, 0.0, m_nInner, m_nRadius, TRUE, nSegments );
}

const char *const *CDisc::ShapeFields( void )
{
    static const char *const astrFields[] = { "radius", "innerRadius", NULL };

    return astrFields;
}

void CDisc::ShapeToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutNumber( oNode, "radius", m_nRadius );
    if ( m_nInner > 0.0 )
        RayJson::PutNumber( oNode, "innerRadius", m_nInner );
}

BOOL CDisc::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    m_nInner = 0.0;
    return RayJson::GetNumber( oNode, "radius", m_nRadius, TRUE ) &&
           RayJson::GetNumber( oNode, "innerRadius", m_nInner, FALSE );
}

void CDisc::ToJson( IASLib::CObjectNode &oNode )
{
    CLocalShape::ToJson( oNode );
}

BOOL CDisc::FromJson( const IASLib::CJsonNode &oNode )
{
    return CLocalShape::FromJson( oNode );
}
