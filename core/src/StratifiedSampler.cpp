#include "Globals.h"

#include "StratifiedSampler.h"
#include "Rays.h"
#include "Color.h"
#include "SceneJson.h"

#include <cstdint>

#define MAXIMUM_GRID    32

namespace
{
        // SplitMix64's finalizer, as in the random sampler.
    uint64_t Mix( uint64_t nValue )
    {
        nValue += 0x9E3779B97F4A7C15ULL;
        nValue = ( nValue ^ ( nValue >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
        nValue = ( nValue ^ ( nValue >> 27 ) ) * 0x94D049BB133111EBULL;
        return nValue ^ ( nValue >> 31 );
    }

    FLT Unit( uint64_t nValue )
    {
        return (FLT)( (double)( nValue >> 11 ) * ( 1.0 / 9007199254740992.0 ) );
    }
}

CStratifiedSampler::CStratifiedSampler( int nGrid, int nSeed ) :
    m_nGrid( ( nGrid >= 1 && nGrid <= MAXIMUM_GRID ) ? nGrid : 4 ),
    m_nSeed( nSeed ),
    m_nLeft( 0 ),
    m_nTop( 0 ),
    m_nWidth( 0 ),
    m_nHeight( 0 ),
    m_nPixel( 0 ),
    m_nRay( 0 )
{
}

long long CStratifiedSampler::Begin( int nLeft, int nTop, int nWidth, int nHeight )
{
    m_nLeft = nLeft;
    m_nTop = nTop;
    m_nWidth = ( nWidth > 0 ) ? nWidth : 0;
    m_nHeight = ( nHeight > 0 ) ? nHeight : 0;
    if ( ( m_nWidth == 0 ) || ( m_nHeight == 0 ) )
        m_nWidth = m_nHeight = 0;
    m_nPixel = 0;
    m_nRay = 0;
    m_oSums.Begin( m_nLeft, m_nTop, m_nWidth, m_nHeight, (uint32_t)( m_nGrid * m_nGrid ) );

    return (long long)( m_nGrid * m_nGrid ) * (long long)m_nWidth * (long long)m_nHeight;
}

BOOL CStratifiedSampler::NextRay( CScene &oScene, CRay &oRay )
{
    if ( m_nPixel >= m_nWidth * m_nHeight )
        return FALSE;

    int         nX = m_nLeft + ( m_nPixel % m_nWidth );
    int         nY = m_nTop + ( m_nPixel / m_nWidth );
    uint64_t    nKey = Mix( (uint64_t)(uint32_t)m_nSeed );

    nKey = Mix( nKey ^ (uint64_t)(uint32_t)nX );
    nKey = Mix( nKey ^ (uint64_t)(uint32_t)nY );
    nKey = Mix( nKey ^ (uint64_t)(uint32_t)m_nRay );

    FLT nU = ( ( m_nRay % m_nGrid ) + Unit( nKey ) ) / m_nGrid;
    FLT nV = ( ( m_nRay / m_nGrid ) + Unit( Mix( nKey ) ) ) / m_nGrid;

    oRay.CreateRayAt( oScene, nX + nU, nY + nV, ( nX * m_nGrid * m_nGrid ) + m_nRay, nY );
    oRay.SetSampleIndex( m_nRay, m_nGrid * m_nGrid );

    if ( ++m_nRay == m_nGrid * m_nGrid )
    {
        m_nRay = 0;
        m_nPixel++;
    }
    return TRUE;
}

void CStratifiedSampler::PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor )
{
    if ( ( nTagX >= 0 ) && ( nTagY >= 0 ) )
        m_oSums.Add( oWriter, nTagX / ( m_nGrid * m_nGrid ), nTagY, oColor, 1 );
}

void CStratifiedSampler::End( void )
{
    m_oSums.End();
}

void CStratifiedSampler::ToJson( IASLib::CObjectNode &oNode ) const
{
    oNode.put( "grid", m_nGrid );
    oNode.put( "seed", m_nSeed );
}

BOOL CStratifiedSampler::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "grid", "seed", NULL };

    if ( ! oNode.isObject() )
        return TRUE;
    return RayJson::OnlyFields( oNode, astrFields ) &&
           RayJson::GetInt( oNode, "grid", m_nGrid, 1, MAXIMUM_GRID, FALSE ) &&
           RayJson::GetInt( oNode, "seed", m_nSeed, 0, 2147483647, FALSE );
}
