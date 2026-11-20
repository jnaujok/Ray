/*************************************************************************
*   CBumpMap                                                             *
*                                                                        *
*   Bump and normal maps. See BumpMap.h.                                 *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "BumpMap.h"
#include "Objects.h"
#include "SceneJson.h"
#include "Texture.h"

#include <cmath>

namespace
{
    const FLT   DEFAULT_DEPTH = 0.05;
    const FLT   DEFAULT_STRENGTH = 1.0;
    const FLT   DEFAULT_STEP = 0.001;

    FLT Dot( const CVector &oA, const CVector &oB )
    {
        CVector oLeft( oA );
        CVector oRight( oB );

        return oLeft.Dot( oRight );
    }

    CVector Combine( FLT nA, const CVector &oA, FLT nB, const CVector &oB )
    {
        CVector oLeft( oA );
        CVector oRight( oB );

        return CVector( nA * oLeft.GetX() + nB * oRight.GetX(), nA * oLeft.GetY() + nB * oRight.GetY(),
                        nA * oLeft.GetZ() + nB * oRight.GetZ() );
    }

        // The texture's value at a point near the surface: its surface
        // coordinates, if the object has them, and the point itself.
    CColor Lookup( const CTexture &oTexture, CObjects &oObject, const CVector &oPoint, BOOL &bHasUV, FLT &nU, FLT &nV )
    {
        STexturePoint oAt;
        CVector       oWhere( oPoint );

        oAt.oPoint = oWhere;
        oAt.nU = oAt.nV = 0.0;
        oAt.bHasUV = oObject.SurfaceCoords( oWhere, oAt.nU, oAt.nV ) ? TRUE : FALSE;
        bHasUV = oAt.bHasUV;
        nU = oAt.nU;
        nV = oAt.nV;
        return oTexture.ColorAt( oAt );
    }

    FLT Height( const CTexture &oTexture, CObjects &oObject, const CVector &oPoint )
    {
        BOOL   bHasUV;
        FLT    nU;
        FLT    nV;
        CColor oColor = Lookup( oTexture, oObject, oPoint, bHasUV, nU, nV );

        return ( oColor.GetR() + oColor.GetG() + oColor.GetB() ) / 3.0;
    }

        // A change in a surface coordinate, taking the short way round where
        // it wraps (a sphere's longitude from 1 back to 0).
    FLT Wrapped( FLT nDelta )
    {
        if ( nDelta > 0.5 )
            return nDelta - 1.0;
        if ( nDelta < -0.5 )
            return nDelta + 1.0;
        return nDelta;
    }
}

CBumpMap::CBumpMap( void ) :
    m_eMode( MODE_HEIGHT ),
    m_nDepth( DEFAULT_DEPTH ),
    m_nStrength( DEFAULT_STRENGTH ),
    m_nStep( DEFAULT_STEP )
{
}

CBumpMap::CBumpMap( const CBumpMap &oSource ) :
    m_pTexture( oSource.m_pTexture ? oSource.m_pTexture->Clone() : NULL ),
    m_eMode( oSource.m_eMode ),
    m_nDepth( oSource.m_nDepth ),
    m_nStrength( oSource.m_nStrength ),
    m_nStep( oSource.m_nStep )
{
}

CBumpMap::~CBumpMap( void )
{
}

void CBumpMap::Perturb( CObjects &oObject, const CVector &oPoint, CVector &oNormal, CRay &oRay ) const
{
    if ( ! m_pTexture )
        return;

    CVector oN( oNormal );

    oN.Normalize();
    if ( oN.Magnitude() == 0.0 )
        return;

        // Two directions along the surface, at right angles to the normal
        // and each other: the axis least along the normal, made square to it.
    FLT     nAX = ABS( oN.GetX() );
    FLT     nAY = ABS( oN.GetY() );
    FLT     nAZ = ABS( oN.GetZ() );
    CVector oAxis = ( nAX <= nAY && nAX <= nAZ ) ? CVector( 1, 0, 0 ) : ( nAY <= nAZ ) ? CVector( 0, 1, 0 ) : CVector( 0, 0, 1 );
    CVector oT1 = oN.Cross( oAxis );

    oT1.Normalize();

    CVector oT2 = oN.Cross( oT1 );
    CVector oP( oPoint );
    CVector oStep1 = oT1 * m_nStep;
    CVector oStep2 = oT2 * m_nStep;
    CVector oPlus1 = oP + oStep1;
    CVector oMinus1 = oP - oStep1;
    CVector oPlus2 = oP + oStep2;
    CVector oMinus2 = oP - oStep2;
    CVector oTilted;

    if ( m_eMode == MODE_HEIGHT )
    {
            // The slope of the heights along each direction, by central
            // differences; the normal tilts down it.
        FLT nSlope1 = m_nDepth * ( Height( *m_pTexture, oObject, oPlus1 ) - Height( *m_pTexture, oObject, oMinus1 ) ) / ( 2.0 * m_nStep );
        FLT nSlope2 = m_nDepth * ( Height( *m_pTexture, oObject, oPlus2 ) - Height( *m_pTexture, oObject, oMinus2 ) ) / ( 2.0 * m_nStep );
        CVector oSlope = Combine( nSlope1, oT1, nSlope2, oT2 );

        oTilted = oN - oSlope;
    }
    else
    {
            // How the surface coordinates change along each direction, and
            // from that, the surface's u and v directions.
        BOOL   bHasUV;
        FLT    nU, nV, nUp1, nVp1, nUm1, nVm1, nUp2, nVp2, nUm2, nVm2;
        CColor oColor = Lookup( *m_pTexture, oObject, oPoint, bHasUV, nU, nV );

        if ( ! bHasUV )
            return;

        BOOL bAll = TRUE;

        Lookup( *m_pTexture, oObject, oPlus1, bHasUV, nUp1, nVp1 );
        bAll = bAll && bHasUV;
        Lookup( *m_pTexture, oObject, oMinus1, bHasUV, nUm1, nVm1 );
        bAll = bAll && bHasUV;
        Lookup( *m_pTexture, oObject, oPlus2, bHasUV, nUp2, nVp2 );
        bAll = bAll && bHasUV;
        Lookup( *m_pTexture, oObject, oMinus2, bHasUV, nUm2, nVm2 );
        bAll = bAll && bHasUV;
        if ( ! bAll )
            return;

        FLT nDU1 = Wrapped( nUp1 - nUm1 ) / ( 2.0 * m_nStep );
        FLT nDV1 = Wrapped( nVp1 - nVm1 ) / ( 2.0 * m_nStep );
        FLT nDU2 = Wrapped( nUp2 - nUm2 ) / ( 2.0 * m_nStep );
        FLT nDV2 = Wrapped( nVp2 - nVm2 ) / ( 2.0 * m_nStep );
        FLT nDeterminant = ( nDU1 * nDV2 ) - ( nDU2 * nDV1 );

        if ( ABS( nDeterminant ) < 1e-12 )
            return;

            // The inverse of [ dU1 dU2; dV1 dV2 ] takes a change in (u, v)
            // to the distances along the two directions.
        CVector oAlongU = Combine( nDV2 / nDeterminant, oT1, -nDV1 / nDeterminant, oT2 );
        CVector oAlongV = Combine( -nDU2 / nDeterminant, oT1, nDU1 / nDeterminant, oT2 );
        CVector oTangent( oAlongU );

        oTangent.Normalize();

        CVector oBitangent = oN.Cross( oTangent );

        if ( Dot( oBitangent, oAlongV ) < 0.0 )
            oBitangent = oBitangent * -1.0;

        FLT     nX = ( ( 2.0 * oColor.GetR() ) - 1.0 ) * m_nStrength;
        FLT     nY = ( ( 2.0 * oColor.GetG() ) - 1.0 ) * m_nStrength;
        FLT     nZ = ( 2.0 * oColor.GetB() ) - 1.0;
        CVector oInPlane = Combine( nX, oTangent, nY, oBitangent );
        CVector oOut = oN * nZ;

        oTilted = oInPlane + oOut;
    }

    oTilted.Normalize();
    if ( oTilted.Magnitude() == 0.0 )
        return;

        // The tilted normal must stay on the same side of the ray as the
        // smooth one, or the surface would be lit from behind.
    CVector oDirection( oRay.GetI(), oRay.GetJ(), oRay.GetK() );
    FLT     nSmooth = Dot( oN, oDirection );
    FLT     nBumped = Dot( oTilted, oDirection );

    if ( ( nSmooth < 0.0 && nBumped >= 0.0 ) || ( nSmooth > 0.0 && nBumped <= 0.0 ) )
        return;

    oNormal.Set( oTilted.GetX(), oTilted.GetY(), oTilted.GetZ() );
}

void CBumpMap::ToJson( IASLib::CObjectNode &oNode ) const
{
    oNode.put( "mode", ( m_eMode == MODE_NORMAL ) ? "normal" : "height" );
    if ( m_pTexture )
    {
        IASLib::CObjectNode *pTexture = oNode.putObject( "texture" );

        pTexture->put( "type", m_pTexture->GetName() );
        m_pTexture->ToJson( *pTexture );
    }
    if ( m_eMode == MODE_NORMAL )
        RayJson::PutNumber( oNode, "strength", m_nStrength );
    else
        RayJson::PutNumber( oNode, "depth", m_nDepth );
    RayJson::PutNumber( oNode, "step", m_nStep );
}

BOOL CBumpMap::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "mode", "texture", "depth", "strength", "step", NULL };
    std::string strMode( "height" );

    if ( ! oNode.isObject() )
        return RayJson::Fail( "must be an object with a \"texture\"" );
    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetString( oNode, "mode", strMode, FALSE ) )
        return FALSE;
    if ( strMode == "height" )
        m_eMode = MODE_HEIGHT;
    else if ( strMode == "normal" )
        m_eMode = MODE_NORMAL;
    else
    {
        RayJson::CWhere oWhere( "mode" );

        return RayJson::Fail( "must be \"height\" or \"normal\", not \"" + strMode + "\"" );
    }
    if ( m_eMode == MODE_NORMAL && oNode.get( "depth" ) )
    {
        RayJson::CWhere oWhere( "depth" );

        return RayJson::Fail( "is for a height map; a normal map takes \"strength\"" );
    }
    if ( m_eMode == MODE_HEIGHT && oNode.get( "strength" ) )
    {
        RayJson::CWhere oWhere( "strength" );

        return RayJson::Fail( "is for a normal map; a height map takes \"depth\"" );
    }
    if ( ! RayJson::GetNumber( oNode, "depth", m_nDepth, FALSE ) ||
         ! RayJson::GetNumber( oNode, "strength", m_nStrength, FALSE ) ||
         ! RayJson::GetNumber( oNode, "step", m_nStep, FALSE ) )
        return FALSE;
    if ( ! std::isfinite( (double)m_nDepth ) || ABS( m_nDepth ) > 1e6 )
    {
        RayJson::CWhere oWhere( "depth" );

        return RayJson::Fail( "must be a number of scene units (negative to invert the bumps)" );
    }
    if ( ! std::isfinite( (double)m_nStrength ) || m_nStrength < 0.0 || m_nStrength > 100.0 )
    {
        RayJson::CWhere oWhere( "strength" );

        return RayJson::Fail( "must be from 0 to 100" );
    }
    if ( ! std::isfinite( (double)m_nStep ) || m_nStep <= 0.0 || m_nStep > 1e6 )
    {
        RayJson::CWhere oWhere( "step" );

        return RayJson::Fail( "must be more than 0" );
    }

    const IASLib::CJsonNode *pTexture = oNode.get( "texture" );

    if ( pTexture == NULL )
        return RayJson::Fail( "needs a \"texture\"" );

    RayJson::CWhere oWhere( "texture" );
    std::string     strType;

    if ( ! RayJson::GetString( *pTexture, "type", strType, TRUE ) )
        return FALSE;

    std::unique_ptr<CTexture> pNew( CTexture::Create( strType.c_str() ) );

    if ( ! pNew )
        return RayJson::Fail( "unknown texture type \"" + strType + "\"" );
    if ( ! pNew->FromJson( *pTexture ) )
        return FALSE;
    m_pTexture.reset( pNew.release() );
    return TRUE;
}
