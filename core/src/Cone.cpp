/*************************************************************************
*   CCone                                                                *
*                                                                        *
*   A cone, or a truncated one, with or without flat caps. See Cone.h.   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Cone.h"
#include "SceneJson.h"

#include <math.h>


CCone::CCone( void ) :
    m_oBase( 0.0, 0.0, 0.0 ),
    m_oTop( 0.0, 1.0, 0.0 ),
    m_nBaseRadius( 1.0 ),
    m_nTopRadius( 0.0 ),
    m_bCapped( TRUE )
{
    m_oMaterials.Add( new CDiffuseMaterial, 1.0 );
    m_oColor.Set( 0.5, 0.5, 0.5 );
    Prepare();
}

CCone::~CCone( void )
{
}


DYNAMIC_IMPLEMENTS( CCone, "cone" );

BOOL CCone::Set( const CVector &oBase, const CVector &oTop, FLT nBaseRadius, FLT nTopRadius, BOOL bCapped )
{
    m_oBase = oBase;
    m_oTop = oTop;
    m_nBaseRadius = nBaseRadius;
    m_nTopRadius = nTopRadius;
    m_bCapped = bCapped;
    return Prepare();
}

BOOL CCone::Prepare( void )
{
    if ( m_nBaseRadius < 0.0 || m_nTopRadius < 0.0 )
        return RayJson::Fail( "a radius can't be negative" );
    if ( m_nBaseRadius == 0.0 && m_nTopRadius == 0.0 )
        return RayJson::Fail( "the radii can't both be 0" );

    m_oAxis = m_oTop - m_oBase;
    m_nHeight = m_oAxis.Magnitude();
    if ( ! ( m_nHeight > 1e-12 ) )
        return RayJson::Fail( "the base and top can't be the same point" );
    m_oAxis = m_oAxis * ( 1.0 / m_nHeight );

    m_nSlope = ( m_nBaseRadius - m_nTopRadius ) / m_nHeight;

        // Across the axis: +x made square to it (or +z, for an axis near x),
        // then the axis's cross product with that. Standing on +y, these are
        // +x and +z, so u runs as it does on a sphere.
    CVector oReference( 1.0, 0.0, 0.0 );

    if ( ABS( m_oAxis.GetX() ) > 0.9 )
        oReference.Set( 0.0, 0.0, 1.0 );

    CVector oAlong = m_oAxis * oReference.Dot( m_oAxis );

    m_oAcross1 = oReference - oAlong;
    m_oAcross1.Normalize();
    m_oAcross2 = m_oAcross1.Cross( m_oAxis );

        // Hits closer than this to where a ray starts are the surface it
        // leaves from, found again through rounding.
    m_nMinimumT = 1e-6 * ( m_nHeight + m_nBaseRadius + m_nTopRadius );
    return TRUE;
}

void CCone::Local( CVector &oPoint, FLT &nAlong, FLT &nAcross1, FLT &nAcross2 )
{
    CVector oOffset = oPoint - m_oBase;

    nAlong = oOffset.Dot( m_oAxis );
    nAcross1 = oOffset.Dot( m_oAcross1 );
    nAcross2 = oOffset.Dot( m_oAcross2 );
}

BOOL CCone::Hit( CRay &oRay, FLT nMinimumT, FLT &nT )
{
    CVector oOrigin( oRay.GetX(), oRay.GetY(), oRay.GetZ() );
    CVector oDirection( oRay.GetI(), oRay.GetJ(), oRay.GetK() );
    FLT     nOA, nO1, nO2;
    FLT     nBest = oRay.GetT();
    BOOL    bFound = FALSE;

    Local( oOrigin, nOA, nO1, nO2 );

    FLT     nDA = oDirection.Dot( m_oAxis );
    FLT     nD1 = oDirection.Dot( m_oAcross1 );
    FLT     nD2 = oDirection.Dot( m_oAcross2 );

        // The side: the distance from the axis equals the radius there,
        // r(y) = baseRadius - slope y, with y between 0 and the height.
        // Squared, that is A t^2 + B t + C = 0.
    FLT     nC0 = m_nBaseRadius - ( m_nSlope * nOA );
    FLT     nA = ( nD1 * nD1 ) + ( nD2 * nD2 ) - ( m_nSlope * m_nSlope * nDA * nDA );
    FLT     nB = 2.0 * ( ( nO1 * nD1 ) + ( nO2 * nD2 ) + ( nC0 * m_nSlope * nDA ) );
    FLT     nC = ( nO1 * nO1 ) + ( nO2 * nO2 ) - ( nC0 * nC0 );
    FLT     anRoots[ 2 ];
    int     nRoots = 0;

    if ( ABS( nA ) > 1e-12 )
    {
        FLT nDet = ( nB * nB ) - ( 4.0 * nA * nC );

        if ( nDet >= 0.0 )
        {
            FLT nSqrt = SQRT( nDet );

            anRoots[ nRoots++ ] = ( -nB - nSqrt ) / ( 2.0 * nA );
            anRoots[ nRoots++ ] = ( -nB + nSqrt ) / ( 2.0 * nA );
        }
    }
    else if ( ABS( nB ) > 1e-12 )
    {
            // Parallel to the side of a cone: one hit.
        anRoots[ nRoots++ ] = -nC / nB;
    }

    for ( int nRoot = 0; nRoot < nRoots; nRoot++ )
    {
        FLT nRootT = anRoots[ nRoot ];
        FLT nY = nOA + ( nRootT * nDA );

        if ( nRootT > nMinimumT && nRootT < nBest && nY >= 0.0 && nY <= m_nHeight )
        {
            nBest = nRootT;
            bFound = TRUE;
        }
    }

        // The caps: discs in the planes square to the axis at each end.
    if ( m_bCapped && nDA != 0.0 )
    {
        for ( int nEnd = 0; nEnd < 2; nEnd++ )
        {
            FLT nRadius = ( nEnd == 0 ) ? m_nBaseRadius : m_nTopRadius;
            FLT nPlane = ( nEnd == 0 ) ? 0.0 : m_nHeight;

            if ( nRadius <= 0.0 )
                continue;

            FLT nCapT = ( nPlane - nOA ) / nDA;
            FLT nX = nO1 + ( nCapT * nD1 );
            FLT nZ = nO2 + ( nCapT * nD2 );

            if ( nCapT > nMinimumT && nCapT < nBest && ( ( nX * nX ) + ( nZ * nZ ) ) <= nRadius * nRadius )
            {
                nBest = nCapT;
                bFound = TRUE;
            }
        }
    }

    if ( bFound )
        nT = nBest;
    return bFound;
}

BOOL CCone::Intersect( CRay &oRay, CVector &oIntersect )
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

BOOL CCone::Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore )
{
    if ( pIgnore != this )
        return Intersect( oRay, oIntersect );
    if ( m_bCapped )
        return FALSE;

    FLT nT;

    if ( ! Hit( oRay, m_nMinimumT, nT ) )
        return FALSE;

    oRay.SetT( nT );
    oIntersect.Set( ( oRay.GetI() * nT ) + oRay.GetX(),
                    ( oRay.GetJ() * nT ) + oRay.GetY(),
                    ( oRay.GetK() * nT ) + oRay.GetZ() );
    return TRUE;
}

int CCone::PartAt( CVector &oPoint )
{
    if ( ! m_bCapped )
        return 0;

    FLT nY, nX, nZ;

    Local( oPoint, nY, nX, nZ );

        // The part the point is nearest to.
    FLT nSide = ABS( SQRT( ( nX * nX ) + ( nZ * nZ ) ) - ( m_nBaseRadius - ( m_nSlope * nY ) ) ) /
                SQRT( 1.0 + ( m_nSlope * m_nSlope ) );
    FLT nBase = ( m_nBaseRadius > 0.0 ) ? ABS( nY ) : 1e300;
    FLT nTop = ( m_nTopRadius > 0.0 ) ? ABS( nY - m_nHeight ) : 1e300;

    if ( nBase < nSide && nBase <= nTop )
        return 1;
    if ( nTop < nSide )
        return 2;
    return 0;
}

CVector CCone::NormalAt( CVector &oPoint )
{
    int     nPart = PartAt( oPoint );
    CVector oNormal;

    if ( nPart == 1 )
        return m_oAxis * -1.0;
    if ( nPart == 2 )
        return m_oAxis;

    FLT nY, nX, nZ;

    Local( oPoint, nY, nX, nZ );

    FLT nRadial = SQRT( ( nX * nX ) + ( nZ * nZ ) );

        // At the point of a cone there is no one normal: use the axis.
    if ( nRadial < 1e-12 )
        return m_oAxis;

        // Straight out from the axis, tipped toward the top by the slope.
    CVector oOut1 = m_oAcross1 * ( nX / nRadial );
    CVector oOut2 = m_oAcross2 * ( nZ / nRadial );
    CVector oTip = m_oAxis * m_nSlope;

    oNormal = oOut1 + oOut2;
    oNormal = oNormal + oTip;
    oNormal.Normalize();
    return oNormal;
}

CColor CCone::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
    CVector oNormal = NormalAt( oIntersect );

    return BaseShade( oScene, oRay, oIntersect, oNormal );
}

BOOL CCone::SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV )
{
    int nPart = PartAt( oPoint );
    FLT nY, nX, nZ;

    Local( oPoint, nY, nX, nZ );

    if ( nPart == 0 )
    {
        nU = 0.5 + ( (FLT)atan2( (double)nZ, (double)nX ) / ( 2.0 * PI ) );
        nV = nY / m_nHeight;
    }
    else
    {
            // Seen from outside: from the top, across1 is to the right and
            // across2 up; from below, across2 is down.
        FLT nRadius = ( nPart == 1 ) ? m_nBaseRadius : m_nTopRadius;

        nU = 0.5 + ( nX / ( 2.0 * nRadius ) );
        nV = ( nPart == 1 ) ? 0.5 - ( nZ / ( 2.0 * nRadius ) ) : 0.5 + ( nZ / ( 2.0 * nRadius ) );
    }
    return TRUE;
}

BOOL CCone::ReadEnds( const IASLib::CJsonNode &oNode )
{
    return RayJson::GetVector( oNode, "base", m_oBase, TRUE ) &&
           RayJson::GetVector( oNode, "top", m_oTop, TRUE ) &&
           RayJson::GetBool( oNode, "capped", m_bCapped, FALSE ) &&
           RayJson::GetColor( oNode, "color", m_oColor, FALSE ) &&
           m_oMaterials.FromJson( oNode, "materials" ) &&
           BumpFromJson( oNode );
}

void CCone::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "base", m_oBase );
    RayJson::PutVector( oNode, "top", m_oTop );
    RayJson::PutNumber( oNode, "baseRadius", m_nBaseRadius );
    RayJson::PutNumber( oNode, "topRadius", m_nTopRadius );
    RayJson::PutBool( oNode, "capped", m_bCapped );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
    BumpToJson( oNode );
}

BOOL CCone::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "base", "top", "baseRadius", "topRadius", "capped", "color", "materials", "bump", NULL };

    m_nTopRadius = 0.0;
    return RayJson::OnlyFields( oNode, astrFields ) &&
           RayJson::GetNumber( oNode, "baseRadius", m_nBaseRadius, TRUE ) &&
           RayJson::GetNumber( oNode, "topRadius", m_nTopRadius, FALSE ) &&
           ReadEnds( oNode ) &&
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

BOOL CCone::GetBounds( CVector &oMinimum, CVector &oMaximum )
{
    FLT anAxis[ 3 ] = { m_oAxis.GetX(), m_oAxis.GetY(), m_oAxis.GetZ() };
    FLT anBase[ 3 ] = { m_oBase.GetX(), m_oBase.GetY(), m_oBase.GetZ() };
    FLT anTop[ 3 ] = { m_oTop.GetX(), m_oTop.GetY(), m_oTop.GetZ() };
    FLT anLow[ 3 ], anHigh[ 3 ];

        // The box around the two end circles: a circle of radius r square to
        // the axis reaches r * sqrt( 1 - axis^2 ) along each scene axis.
    for ( int nAxis = 0; nAxis < 3; nAxis++ )
    {
        FLT nSpread = 1.0 - ( anAxis[ nAxis ] * anAxis[ nAxis ] );
        FLT nAcross = SQRT( ( nSpread > 0.0 ) ? nSpread : 0.0 );
        FLT nBaseLow = anBase[ nAxis ] - ( m_nBaseRadius * nAcross );
        FLT nBaseHigh = anBase[ nAxis ] + ( m_nBaseRadius * nAcross );
        FLT nTopLow = anTop[ nAxis ] - ( m_nTopRadius * nAcross );
        FLT nTopHigh = anTop[ nAxis ] + ( m_nTopRadius * nAcross );

        anLow[ nAxis ] = ( nBaseLow < nTopLow ) ? nBaseLow : nTopLow;
        anHigh[ nAxis ] = ( nBaseHigh > nTopHigh ) ? nBaseHigh : nTopHigh;
    }
    PadBounds( anLow, anHigh, oMinimum, oMaximum );
    return TRUE;
}
