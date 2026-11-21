#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "BoxSampler.h"
#include "Rays.h"
#include "Color.h"

CBoxSampler::CBoxSampler( void ) :
    m_nLeft( 0 ),
    m_nTop( 0 ),
    m_nWidth( 0 ),
    m_nHeight( 0 ),
    m_nPixel( 0 ),
    m_nRay( 0 )
{
}

long long CBoxSampler::Begin( int nLeft, int nTop, int nWidth, int nHeight )
{
    m_nLeft = nLeft;
    m_nTop = nTop;
    m_nWidth = ( nWidth > 0 ) ? nWidth : 0;
    m_nHeight = ( nHeight > 0 ) ? nHeight : 0;
    if ( ( m_nWidth == 0 ) || ( m_nHeight == 0 ) )
        m_nWidth = m_nHeight = 0;
    m_nPixel = 0;
    m_nRay = 0;
    m_oSums.Begin( m_nLeft, m_nTop, m_nWidth, m_nHeight, 4 );

    return 4LL * (long long)m_nWidth * (long long)m_nHeight;
}

BOOL CBoxSampler::NextRay( CScene &oScene, CRay &oRay )
{
    if ( m_nPixel >= m_nWidth * m_nHeight )
        return FALSE;

    int nX = m_nLeft + ( m_nPixel % m_nWidth );
    int nY = m_nTop + ( m_nPixel / m_nWidth );
    int nI = m_nRay & 1;
    int nJ = m_nRay >> 1;

    oRay.CreateRayAt( oScene, nX + 0.25 + ( 0.5 * nI ), nY + 0.25 + ( 0.5 * nJ ), ( 2 * nX ) + nI, ( 2 * nY ) + nJ );
    oRay.SetSampleIndex( m_nRay, 4 );

    if ( ++m_nRay == 4 )
    {
        m_nRay = 0;
        m_nPixel++;
    }
    return TRUE;
}

void CBoxSampler::PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor )
{
    if ( ( nTagX >= 0 ) && ( nTagY >= 0 ) )
        m_oSums.Add( oWriter, nTagX / 2, nTagY / 2, oColor, 1 );
}

void CBoxSampler::End( void )
{
    m_oSums.End();
}
