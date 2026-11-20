/*************************************************************************
*   CAdaptiveSampler                                                     *
*                                                                        *
*   Rays where they are needed. See AdaptiveSampler.h.                   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "AdaptiveSampler.h"
#include "Color.h"
#include "PixelWriter.h"
#include "Rays.h"
#include "SceneJson.h"

#include <math.h>

#define MAXIMUM_RAYS        4096
    // A brightness above this counts as this, so a pixel with a tiny, very
    // bright highlight in it still settles.
#define BRIGHTNESS_CAP      4.0

namespace
{
        // SplitMix64's finalizer, as in the other samplers.
    uint64_t Mix( uint64_t nValue )
    {
        nValue += 0x9E3779B97F4A7C15ULL;
        nValue = ( nValue ^ ( nValue >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
        nValue = ( nValue ^ ( nValue >> 27 ) ) * 0x94D049BB133111EBULL;
        return nValue ^ ( nValue >> 31 );
    }

    double Unit( uint64_t nValue )
    {
        return (double)( nValue >> 11 ) * ( 1.0 / 9007199254740992.0 );
    }

        // The R2 sequence's steps: 1 / g and 1 / g^2, where g^3 = g + 1.
    const double s_dStepU = 0.75487766624669276005;
    const double s_dStepV = 0.56984029099805326591;
}

CAdaptiveSampler::CAdaptiveSampler( void ) :
    m_nMinimum( 9 ),
    m_nMaximum( 64 ),
    m_nThreshold( 0.01 ),
    m_nSeed( 0 ),
    m_nLeft( 0 ),
    m_nTop( 0 ),
    m_nWidth( 0 ),
    m_nHeight( 0 ),
    m_nPixel( 0 ),
    m_nRays( 0 ),
    m_dBrightness( 0.0 ),
    m_dSquares( 0.0 )
{
    m_adSum[ 0 ] = m_adSum[ 1 ] = m_adSum[ 2 ] = 0.0;
    m_adShift[ 0 ] = m_adShift[ 1 ] = 0.0;
}

CPixelSampler *CAdaptiveSampler::Clone( void ) const
{
    CAdaptiveSampler *pCopy = new CAdaptiveSampler;

    pCopy->m_nMinimum = m_nMinimum;
    pCopy->m_nMaximum = m_nMaximum;
    pCopy->m_nThreshold = m_nThreshold;
    pCopy->m_nSeed = m_nSeed;
    return pCopy;
}

long long CAdaptiveSampler::Begin( int nLeft, int nTop, int nWidth, int nHeight )
{
    m_nLeft = nLeft;
    m_nTop = nTop;
    m_nWidth = ( nWidth > 0 && nHeight > 0 ) ? nWidth : 0;
    m_nHeight = ( nWidth > 0 && nHeight > 0 ) ? nHeight : 0;
    m_nPixel = 0;
    StartPixel();

        // At most; how many it makes depends on the image.
    return (long long)m_nMaximum * (long long)m_nWidth * (long long)m_nHeight;
}

void CAdaptiveSampler::StartPixel( void )
{
    m_nRays = 0;
    m_adSum[ 0 ] = m_adSum[ 1 ] = m_adSum[ 2 ] = 0.0;
    m_dBrightness = m_dSquares = 0.0;
    if ( m_nWidth > 0 )
    {
        int         nX = m_nLeft + ( m_nPixel % m_nWidth );
        int         nY = m_nTop + ( m_nPixel / m_nWidth );
        uint64_t    nKey = Mix( Mix( Mix( (uint64_t)(uint32_t)m_nSeed ) ^ (uint64_t)(uint32_t)nX ) ^ (uint64_t)(uint32_t)nY );

        m_adShift[ 0 ] = Unit( nKey );
        m_adShift[ 1 ] = Unit( Mix( nKey ) );
    }
}

BOOL CAdaptiveSampler::NextRay( CScene &oScene, CRay &oRay )
{
    if ( m_nWidth == 0 || m_nPixel >= m_nWidth * m_nHeight )
        return FALSE;

    int     nX = m_nLeft + ( m_nPixel % m_nWidth );
    int     nY = m_nTop + ( m_nPixel / m_nWidth );
    double  dU = m_adShift[ 0 ] + ( s_dStepU * ( m_nRays + 1 ) );
    double  dV = m_adShift[ 1 ] + ( s_dStepV * ( m_nRays + 1 ) );

    dU -= floor( dU );
    dV -= floor( dV );
    oRay.CreateRayAt( oScene, nX + (FLT)dU, nY + (FLT)dV, nX, nY );
    return TRUE;
}

void CAdaptiveSampler::PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor )
{
    CColor  oSample( oColor );
    double  dBrightness = ( 0.2126 * (double)oSample.GetR() ) + ( 0.7152 * (double)oSample.GetG() ) + ( 0.0722 * (double)oSample.GetB() );

    if ( ! ( dBrightness > 0.0 ) )
        dBrightness = 0.0;
    else if ( dBrightness > BRIGHTNESS_CAP )
        dBrightness = BRIGHTNESS_CAP;

    m_adSum[ 0 ] += (double)oSample.GetR();
    m_adSum[ 1 ] += (double)oSample.GetG();
    m_adSum[ 2 ] += (double)oSample.GetB();
    m_dBrightness += dBrightness;
    m_dSquares += dBrightness * dBrightness;
    m_nRays++;

        // Done once it has its least, and the error in its average
        // brightness, sqrt( variance / n ), is under the threshold, or it
        // has its most.
    BOOL bDone = ( m_nRays >= m_nMaximum ) ? TRUE : FALSE;

    if ( ! bDone && m_nRays >= m_nMinimum )
    {
        double dMean = m_dBrightness / m_nRays;
        double dVariance = ( m_dSquares - ( dMean * m_dBrightness ) ) / ( m_nRays - 1 );

        if ( dVariance < 0.0 )
            dVariance = 0.0;
        bDone = ( sqrt( dVariance / m_nRays ) <= (double)m_nThreshold ) ? TRUE : FALSE;
    }

    if ( bDone )
    {
        oWriter.PutPixel( nTagX, nTagY, CColor( (FLT)( m_adSum[ 0 ] / m_nRays ), (FLT)( m_adSum[ 1 ] / m_nRays ),
                                                (FLT)( m_adSum[ 2 ] / m_nRays ) ) );
        m_nPixel++;
        StartPixel();
    }
}

void CAdaptiveSampler::ToJson( IASLib::CObjectNode &oNode ) const
{
    oNode.put( "min", m_nMinimum );
    oNode.put( "max", m_nMaximum );
    RayJson::PutNumber( oNode, "threshold", m_nThreshold );
    oNode.put( "seed", m_nSeed );
}

BOOL CAdaptiveSampler::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "min", "max", "threshold", "seed", NULL };
    int nMinimum = m_nMinimum;
    int nMaximum = m_nMaximum;
    FLT nThreshold = m_nThreshold;

    if ( ! oNode.isObject() )
        return TRUE;
    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetInt( oNode, "min", nMinimum, 2, MAXIMUM_RAYS, FALSE ) ||
         ! RayJson::GetInt( oNode, "max", nMaximum, 2, MAXIMUM_RAYS, FALSE ) ||
         ! RayJson::GetNumber( oNode, "threshold", nThreshold, FALSE ) ||
         ! RayJson::GetInt( oNode, "seed", m_nSeed, 0, 2147483647, FALSE ) )
        return FALSE;
    if ( nMaximum < nMinimum )
        return RayJson::Fail( "max can't be less than min" );
    if ( ! ( nThreshold >= 0.0 ) )
        return RayJson::Fail( "threshold can't be negative" );
    m_nMinimum = nMinimum;
    m_nMaximum = nMaximum;
    m_nThreshold = nThreshold;
    return TRUE;
}
