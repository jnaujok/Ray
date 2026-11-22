#include "Globals.h"

#include "RandomSampler.h"
#include "Rays.h"
#include "Color.h"
#include "SceneJson.h"

#include <cstdint>

#define MAXIMUM_SAMPLES     1024

namespace
{
        // SplitMix64's finalizer: spreads every input bit over the output.
    uint64_t Mix( uint64_t nValue )
    {
        nValue += 0x9E3779B97F4A7C15ULL;
        nValue = ( nValue ^ ( nValue >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
        nValue = ( nValue ^ ( nValue >> 27 ) ) * 0x94D049BB133111EBULL;
        return nValue ^ ( nValue >> 31 );
    }

        // A number from 0 up to (not including) 1, from the top 53 bits.
    FLT Unit( uint64_t nValue )
    {
        return (FLT)( (double)( nValue >> 11 ) * ( 1.0 / 9007199254740992.0 ) );
    }
}

CRandomSampler::CRandomSampler( int nSamples, int nSeed ) :
    m_nSamples( 4 ),
    m_nSeed( nSeed ),
    m_nLeft( 0 ),
    m_nTop( 0 ),
    m_nWidth( 0 ),
    m_nHeight( 0 ),
    m_nPixel( 0 ),
    m_nRay( 0 )
{
    SetSamples( nSamples );
}

BOOL CRandomSampler::SetSamples( int nSamples )
{
    if ( ( nSamples < 1 ) || ( nSamples > MAXIMUM_SAMPLES ) )
        return FALSE;
    m_nSamples = nSamples;
    return TRUE;
}

void CRandomSampler::Offset( int nX, int nY, int nRay, FLT &nU, FLT &nV ) const
{
    uint64_t nKey = Mix( (uint64_t)(uint32_t)m_nSeed );

    nKey = Mix( nKey ^ (uint64_t)(uint32_t)nX );
    nKey = Mix( nKey ^ (uint64_t)(uint32_t)nY );
    nKey = Mix( nKey ^ (uint64_t)(uint32_t)nRay );
    nU = Unit( nKey );
    nV = Unit( Mix( nKey ) );
}

long long CRandomSampler::Begin( int nLeft, int nTop, int nWidth, int nHeight )
{
    m_nLeft = nLeft;
    m_nTop = nTop;
    m_nWidth = ( nWidth > 0 ) ? nWidth : 0;
    m_nHeight = ( nHeight > 0 ) ? nHeight : 0;
    if ( ( m_nWidth == 0 ) || ( m_nHeight == 0 ) )
        m_nWidth = m_nHeight = 0;
    m_nPixel = 0;
    m_nRay = 0;

        // Weights: the center N, each random ray 1, so the center is half
        // of the total 2N.
    m_oSums.Begin( m_nLeft, m_nTop, m_nWidth, m_nHeight, (uint32_t)( 2 * m_nSamples ) );

    return (long long)( m_nSamples + 1 ) * (long long)m_nWidth * (long long)m_nHeight;
}

BOOL CRandomSampler::NextRay( CScene &oScene, CRay &oRay )
{
    if ( m_nPixel >= m_nWidth * m_nHeight )
        return FALSE;

    int nX = m_nLeft + ( m_nPixel % m_nWidth );
    int nY = m_nTop + ( m_nPixel / m_nWidth );
    int nTagX = ( nX * ( m_nSamples + 1 ) ) + m_nRay;

    if ( m_nRay == 0 )
        oRay.CreateRayAt( oScene, nX + 0.5, nY + 0.5, nTagX, nY );
    else
    {
        FLT nU, nV;

        Offset( nX, nY, m_nRay, nU, nV );
        oRay.CreateRayAt( oScene, nX + nU, nY + nV, nTagX, nY );
    }
    oRay.SetSampleIndex( m_nRay, m_nSamples + 1 );

    if ( ++m_nRay > m_nSamples )
    {
        m_nRay = 0;
        m_nPixel++;
    }
    return TRUE;
}

void CRandomSampler::PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor )
{
    if ( ( nTagX < 0 ) || ( nTagY < 0 ) )
        return;

    int nRay = nTagX % ( m_nSamples + 1 );

    m_oSums.Add( oWriter, nTagX / ( m_nSamples + 1 ), nTagY, oColor, ( nRay == 0 ) ? (uint32_t)m_nSamples : 1u );
}

void CRandomSampler::End( void )
{
    m_oSums.End();
}

void CRandomSampler::ToJson( IASLib::CObjectNode &oNode ) const
{
    oNode.put( "samples", m_nSamples );
    oNode.put( "seed", m_nSeed );
}

BOOL CRandomSampler::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "samples", "seed", NULL };
    int nSamples = m_nSamples;
    int nSeed = m_nSeed;

    if ( ! oNode.isObject() )
        return TRUE;
    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetInt( oNode, "samples", nSamples, 1, MAXIMUM_SAMPLES, FALSE ) ||
         ! RayJson::GetInt( oNode, "seed", nSeed, 0, 2147483647, FALSE ) )
        return FALSE;
    SetSamples( nSamples );
    m_nSeed = nSeed;
    return TRUE;
}
