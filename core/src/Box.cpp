/*************************************************************************
*   CBox                                                                 *
*                                                                        *
*   A rectangular box, turned any way. See Box.h.                        *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Box.h"
#include "SceneJson.h"

#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

    // Turns anVector about x, then y, then z, by the angles (radians).
static void RotateXYZ( FLT anVector[ 3 ], FLT nX, FLT nY, FLT nZ )
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

CBox::CBox( void ) :
    m_oCenter( 0.0, 0.0, 0.0 ),
    m_oSize( 1.0, 1.0, 1.0 ),
    m_oRotation( 0.0, 0.0, 0.0 )
{
    m_oMaterials.Add( new CDiffuseMaterial, 1.0 );
    m_oColor.Set( 0.5, 0.5, 0.5 );
    Prepare();
}

CBox::~CBox( void )
{
}

#ifdef RAY_GUI
void CBox::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hBox;

    hBox = pTree->AddObject( this, 10, "Box", hUnder );
    CObjects::MatToTree( pTree, hBox );
}
#endif

DYNAMIC_IMPLEMENTS( CBox, "box" );

BOOL CBox::Set( const CVector &oCenter, const CVector &oSize, const CVector &oRotationDegrees )
{
    m_oCenter = oCenter;
    m_oSize = oSize;
    m_oRotation = oRotationDegrees;
    return Prepare();
}

BOOL CBox::Prepare( void )
{
    if ( ! ( m_oSize.GetX() > 0.0 && m_oSize.GetY() > 0.0 && m_oSize.GetZ() > 0.0 ) )
        return RayJson::Fail( "every size must be more than 0" );

    m_anHalf[ 0 ] = m_oSize.GetX() / 2.0;
    m_anHalf[ 1 ] = m_oSize.GetY() / 2.0;
    m_anHalf[ 2 ] = m_oSize.GetZ() / 2.0;

    FLT nToRadians = PI / 180.0;

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        FLT anAxis[ 3 ] = { 0.0, 0.0, 0.0 };

        anAxis[ nAxis ] = 1.0;
        RotateXYZ( anAxis, m_oRotation.GetX() * nToRadians, m_oRotation.GetY() * nToRadians, m_oRotation.GetZ() * nToRadians );
        m_aoAxes[ nAxis ].Set( anAxis[ 0 ], anAxis[ 1 ], anAxis[ 2 ] );
    }
    return TRUE;
}

void CBox::Local( CVector &oPoint, FLT anLocal[ 3 ] )
{
    CVector oOffset = oPoint - m_oCenter;

    for ( int nAxis = 0; nAxis < 3; nAxis++ )
        anLocal[ nAxis ] = oOffset.Dot( m_aoAxes[ nAxis ] ) / m_anHalf[ nAxis ];
}

BOOL CBox::Intersect( CRay &oRay, CVector &oIntersect )
{
    CVector oOrigin( oRay.GetX(), oRay.GetY(), oRay.GetZ() );
    CVector oDirection( oRay.GetI(), oRay.GetJ(), oRay.GetK() );
    CVector oOffset = oOrigin - m_oCenter;
    FLT     nNear = -1e300;
    FLT     nFar = 1e300;

        // Between each pair of opposite faces (a slab), the ray is inside
        // for a stretch of t; it is in the box where all three overlap.
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        FLT nStart = oOffset.Dot( m_aoAxes[ nAxis ] );
        FLT nStep = oDirection.Dot( m_aoAxes[ nAxis ] );
        FLT nHalf = m_anHalf[ nAxis ];

        if ( nStep == 0.0 )
        {
            if ( nStart < -nHalf || nStart > nHalf )
                return FALSE;
            continue;
        }

        FLT nT1 = ( -nHalf - nStart ) / nStep;
        FLT nT2 = ( nHalf - nStart ) / nStep;

        if ( nT1 > nT2 )
        {
            FLT nSwap = nT1;

            nT1 = nT2;
            nT2 = nSwap;
        }
        if ( nT1 > nNear )
            nNear = nT1;
        if ( nT2 < nFar )
            nFar = nT2;
        if ( nNear > nFar )
            return FALSE;
    }

        // From outside, where the ray goes in; from inside, where it leaves.
    FLT nT = ( nNear > 0.0 ) ? nNear : nFar;

    if ( nT > 0.0 && nT < oRay.GetT() )
    {
        oRay.SetT( nT );
        oIntersect.Set( ( oRay.GetI() * nT ) + oRay.GetX(),
                        ( oRay.GetJ() * nT ) + oRay.GetY(),
                        ( oRay.GetK() * nT ) + oRay.GetZ() );
        return TRUE;
    }
    return FALSE;
}

void CBox::FaceAt( CVector &oPoint, int &nAxis, int &nSign, FLT anLocal[ 3 ] )
{
    Local( oPoint, anLocal );

    nAxis = 0;
    for ( int nTry = 1; nTry < 3; nTry++ )
    {
        if ( ABS( anLocal[ nTry ] ) > ABS( anLocal[ nAxis ] ) )
            nAxis = nTry;
    }
    nSign = ( anLocal[ nAxis ] < 0.0 ) ? -1 : 1;
}

CVector CBox::NormalAt( CVector &oPoint )
{
    int nAxis, nSign;
    FLT anLocal[ 3 ];

    FaceAt( oPoint, nAxis, nSign, anLocal );
    return m_aoAxes[ nAxis ] * (FLT)nSign;
}

CColor CBox::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
    CVector oNormal = NormalAt( oIntersect );

    return BaseShade( oScene, oRay, oIntersect, oNormal );
}

BOOL CBox::SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV )
{
    int nAxis, nSign;
    FLT anLocal[ 3 ];
    FLT nAcross, nUp;

    FaceAt( oPoint, nAxis, nSign, anLocal );

        // Across and up on each face, as seen from outside it.
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
    return TRUE;
}

void CBox::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "center", m_oCenter );
    if ( m_oSize.GetX() == m_oSize.GetY() && m_oSize.GetY() == m_oSize.GetZ() )
        RayJson::PutNumber( oNode, "size", m_oSize.GetX() );
    else
        RayJson::PutVector( oNode, "size", m_oSize );
    RayJson::PutVector( oNode, "rotation", m_oRotation );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
    BumpToJson( oNode );
}

BOOL CBox::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "center", "size", "rotation", "color", "materials", "bump", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetVector( oNode, "center", m_oCenter, TRUE ) )
        return FALSE;

        // A size is one number for a cube, or [x, y, z].
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

    return RayJson::GetVector( oNode, "rotation", m_oRotation, FALSE ) &&
           RayJson::GetColor( oNode, "color", m_oColor, FALSE ) &&
           m_oMaterials.FromJson( oNode, "materials" ) &&
           BumpFromJson( oNode ) &&
           Prepare();
}

    // A box a little larger than [anLow, anHigh], so hits a hair outside the
    // exact shape (as some tests allow) are inside it.
static void PadBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ], CVector &oMinimum, CVector &oMaximum )
{
    FLT nMargin = 1e-6 * ( 1.0 + ( anHigh[ 0 ] - anLow[ 0 ] ) + ( anHigh[ 1 ] - anLow[ 1 ] ) + ( anHigh[ 2 ] - anLow[ 2 ] ) );

    oMinimum.Set( anLow[ 0 ] - nMargin, anLow[ 1 ] - nMargin, anLow[ 2 ] - nMargin );
    oMaximum.Set( anHigh[ 0 ] + nMargin, anHigh[ 1 ] + nMargin, anHigh[ 2 ] + nMargin );
}

BOOL CBox::GetBounds( CVector &oMinimum, CVector &oMaximum )
{
    FLT anCenter[ 3 ] = { m_oCenter.GetX(), m_oCenter.GetY(), m_oCenter.GetZ() };
    FLT anLow[ 3 ], anHigh[ 3 ];

        // How far the turned box reaches along each scene axis.
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        FLT nReach = 0.0;

        for ( int nOwn = 0; nOwn < 3; nOwn++ )
        {
            CVector &oOwn = m_aoAxes[ nOwn ];
            FLT      nAlong = ( nAxis == 0 ) ? oOwn.GetX() : ( nAxis == 1 ) ? oOwn.GetY() : oOwn.GetZ();

            nReach += ABS( nAlong ) * m_anHalf[ nOwn ];
        }
        anLow[ nAxis ] = anCenter[ nAxis ] - nReach;
        anHigh[ nAxis ] = anCenter[ nAxis ] + nReach;
    }
    PadBounds( anLow, anHigh, oMinimum, oMaximum );
    return TRUE;
}

BOOL CBox::SurfaceNormal( CVector &oPoint, CVector &oNormal )
{
    oNormal = NormalAt( oPoint );
    return TRUE;
}
