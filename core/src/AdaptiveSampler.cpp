/*************************************************************************
*   CAdaptiveSampler                                                     *
*                                                                        *
*   Rays where they are needed. See AdaptiveSampler.h.                   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#include "Globals.h"

#include "AdaptiveSampler.h"
#include "Color.h"
#include "PixelWriter.h"
#include "RayCountMap.h"
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
    m_dSquares( 0.0 ),
    m_nConfidence( 0.0 ),
    m_bChannels( FALSE )
{
    m_adSum[ 0 ] = m_adSum[ 1 ] = m_adSum[ 2 ] = 0.0;
    m_adShift[ 0 ] = m_adShift[ 1 ] = 0.0;
    for ( int nChannel = 0; nChannel < 3; nChannel++ )
        m_adChannel[ nChannel ] = m_adChannelSquares[ nChannel ] = 0.0;
}

double CAdaptiveSampler::StudentT( double dConfidence, int nFreedom )
{
    const double dPi = 3.14159265358979323846;
    double dP = 0.5 + ( 0.5 * dConfidence );        // the upper tail's quantile

    if ( nFreedom < 1 )
        nFreedom = 1;
    if ( nFreedom == 1 )
        return tan( dPi * ( dP - 0.5 ) );
    if ( nFreedom == 2 )
        return ( ( 2.0 * dP ) - 1.0 ) / sqrt( 2.0 * dP * ( 1.0 - dP ) );

        // The normal quantile (Acklam's rational approximation).
    static const double a[] = { -3.969683028665376e+01, 2.209460984245205e+02, -2.759285104469687e+02,
                                1.383577518672690e+02, -3.066479806614716e+01, 2.506628277459239e+00 };
    static const double b[] = { -5.447609879822406e+01, 1.615858368580409e+02, -1.556989798598866e+02,
                                6.680131188771972e+01, -1.328068155288572e+01 };
    static const double c[] = { -7.784894002430293e-03, -3.223964580411365e-01, -2.400758277161838e+00,
                                -2.549732539343734e+00, 4.374664141464968e+00, 2.938163982698783e+00 };
    static const double d[] = { 7.784695709041462e-03, 3.224671290700398e-01, 2.445134137142996e+00,
                                3.754408661907416e+00 };
    double dZ;

    if ( dP < 0.97575 )
    {
        double dQ = dP - 0.5;
        double dR = dQ * dQ;

        dZ = ( ( ( ( ( a[ 0 ] * dR + a[ 1 ] ) * dR + a[ 2 ] ) * dR + a[ 3 ] ) * dR + a[ 4 ] ) * dR + a[ 5 ] ) * dQ /
             ( ( ( ( ( b[ 0 ] * dR + b[ 1 ] ) * dR + b[ 2 ] ) * dR + b[ 3 ] ) * dR + b[ 4 ] ) * dR + 1.0 );
    }
    else
    {
        double dQ = sqrt( -2.0 * log( 1.0 - dP ) );

        dZ = -( ( ( ( ( c[ 0 ] * dQ + c[ 1 ] ) * dQ + c[ 2 ] ) * dQ + c[ 3 ] ) * dQ + c[ 4 ] ) * dQ + c[ 5 ] ) /
             ( ( ( ( d[ 0 ] * dQ + d[ 1 ] ) * dQ + d[ 2 ] ) * dQ + d[ 3 ] ) * dQ + 1.0 );
    }

        // Cornish-Fisher: Student's t from the normal quantile.
    double dN = (double)nFreedom;
    double dZ2 = dZ * dZ;
    double dZ3 = dZ2 * dZ;
    double dZ5 = dZ3 * dZ2;
    double dZ7 = dZ5 * dZ2;
    double dZ9 = dZ7 * dZ2;

    return dZ + ( ( dZ3 + dZ ) / ( 4.0 * dN ) )
              + ( ( 5.0 * dZ5 + 16.0 * dZ3 + 3.0 * dZ ) / ( 96.0 * dN * dN ) )
              + ( ( 3.0 * dZ7 + 19.0 * dZ5 + 17.0 * dZ3 - 15.0 * dZ ) / ( 384.0 * dN * dN * dN ) )
              + ( ( 79.0 * dZ9 + 776.0 * dZ7 + 1482.0 * dZ5 - 1920.0 * dZ3 - 945.0 * dZ ) /
                  ( 92160.0 * dN * dN * dN * dN ) );
}

void CAdaptiveSampler::MakeFactors( void )
{
    m_adFactor.clear();
    if ( m_nConfidence > 0.0 )
    {
            // By ray count: the factor for n rays is t at n - 1 degrees of freedom.
        m_adFactor.resize( (size_t)m_nMaximum + 1, 1.0 );
        for ( int nRays = 2; nRays <= m_nMaximum; nRays++ )
            m_adFactor[ (size_t)nRays ] = StudentT( (double)m_nConfidence, nRays - 1 );
    }
}

CPixelSampler *CAdaptiveSampler::Clone( void ) const
{
    CAdaptiveSampler *pCopy = new CAdaptiveSampler;

    pCopy->m_nMinimum = m_nMinimum;
    pCopy->m_nMaximum = m_nMaximum;
    pCopy->m_nThreshold = m_nThreshold;
    pCopy->m_nSeed = m_nSeed;
    pCopy->m_nConfidence = m_nConfidence;
    pCopy->m_bChannels = m_bChannels;
    pCopy->m_adFactor = m_adFactor;
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
    for ( int nChannel = 0; nChannel < 3; nChannel++ )
        m_adChannel[ nChannel ] = m_adChannelSquares[ nChannel ] = 0.0;
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
        // Of at most m_nMaximum: a pixel that stops early has taken some of
        // the strata, in a shuffled order, so its wavelengths still spread.
    oRay.SetSampleIndex( m_nRays, m_nMaximum );
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
    if ( m_bChannels )
    {
        double adValue[ 3 ] = { (double)oSample.GetR(), (double)oSample.GetG(), (double)oSample.GetB() };

        for ( int nChannel = 0; nChannel < 3; nChannel++ )
        {
            double dValue = adValue[ nChannel ];

            if ( ! ( dValue > 0.0 ) )
                dValue = 0.0;
            else if ( dValue > BRIGHTNESS_CAP )
                dValue = BRIGHTNESS_CAP;
            m_adChannel[ nChannel ] += dValue;
            m_adChannelSquares[ nChannel ] += dValue * dValue;
        }
    }
    m_nRays++;

        // Done once it has its least, and the error in its average
        // brightness, sqrt( variance / n ), is under the threshold, or it
        // has its most.
    BOOL bDone = ( m_nRays >= m_nMaximum ) ? TRUE : FALSE;

    if ( ! bDone && m_nRays >= m_nMinimum )
    {
        if ( ! m_bChannels && m_adFactor.empty() )
        {
                // The original rule, kept exactly.
            double dMean = m_dBrightness / m_nRays;
            double dVariance = ( m_dSquares - ( dMean * m_dBrightness ) ) / ( m_nRays - 1 );

            if ( dVariance < 0.0 )
                dVariance = 0.0;
            bDone = ( sqrt( dVariance / m_nRays ) <= (double)m_nThreshold ) ? TRUE : FALSE;
        }
        else
        {
                // The largest variance among the measured values, times
                // Student's t when a confidence is set.
            double  dWorst = 0.0;
            int     nValues = m_bChannels ? 3 : 1;

            for ( int nValue = 0; nValue < nValues; nValue++ )
            {
                double dSum = m_bChannels ? m_adChannel[ nValue ] : m_dBrightness;
                double dSquares = m_bChannels ? m_adChannelSquares[ nValue ] : m_dSquares;
                double dMean = dSum / m_nRays;
                double dVariance = ( dSquares - ( dMean * dSum ) ) / ( m_nRays - 1 );

                if ( dVariance > dWorst )
                    dWorst = dVariance;
            }

            double dError = sqrt( dWorst / m_nRays );

            if ( ! m_adFactor.empty() )
                dError *= m_adFactor[ (size_t)m_nRays ];
            bDone = ( dError <= (double)m_nThreshold ) ? TRUE : FALSE;
        }
    }

    if ( bDone )
    {
        CRayCountMap::Record( nTagX, nTagY, m_nRays );
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
    if ( m_nConfidence > 0.0 )
        RayJson::PutNumber( oNode, "confidence", m_nConfidence );
    if ( m_bChannels )
        oNode.put( "channels", "rgb" );
}

BOOL CAdaptiveSampler::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "min", "max", "threshold", "seed", "confidence", "channels", NULL };
    int nMinimum = m_nMinimum;
    int nMaximum = m_nMaximum;
    FLT nThreshold = m_nThreshold;
    FLT nConfidence = m_nConfidence;
    std::string strChannels = m_bChannels ? "rgb" : "brightness";

    if ( ! oNode.isObject() )
        return TRUE;
    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetInt( oNode, "min", nMinimum, 2, MAXIMUM_RAYS, FALSE ) ||
         ! RayJson::GetInt( oNode, "max", nMaximum, 2, MAXIMUM_RAYS, FALSE ) ||
         ! RayJson::GetNumber( oNode, "threshold", nThreshold, FALSE ) ||
         ! RayJson::GetInt( oNode, "seed", m_nSeed, 0, 2147483647, FALSE ) ||
         ! RayJson::GetNumber( oNode, "confidence", nConfidence, FALSE ) ||
         ! RayJson::GetString( oNode, "channels", strChannels, FALSE ) )
        return FALSE;
    if ( nConfidence != 0.0 && ! ( nConfidence >= 0.5 && nConfidence <= 0.999 ) )
        return RayJson::Fail( "confidence must be 0 (off) or between 0.5 and 0.999" );
    if ( strChannels != "brightness" && strChannels != "rgb" )
        return RayJson::Fail( "channels must be \"brightness\" or \"rgb\"" );
    if ( nMaximum < nMinimum )
        return RayJson::Fail( "max can't be less than min" );
    if ( ! ( nThreshold >= 0.0 ) )
        return RayJson::Fail( "threshold can't be negative" );
    m_nMinimum = nMinimum;
    m_nMaximum = nMaximum;
    m_nThreshold = nThreshold;
    m_nConfidence = nConfidence;
    m_bChannels = ( strChannels == "rgb" ) ? TRUE : FALSE;
    MakeFactors();
    return TRUE;
}
