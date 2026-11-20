#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "CornerSampler.h"
#include "Rays.h"
#include "Color.h"
#include "PixelWriter.h"

    // The rays averaged into each pixel.
#define PIXEL_SAMPLES      5

CCornerSampler::CCornerSampler( void ) :
    m_nLeft( 0 ),
    m_nTop( 0 ),
    m_nWidth( 0 ),
    m_nHeight( 0 ),
    m_nGridX( 0 ),
    m_nGridY( 0 )
{
}

long long CCornerSampler::Begin( int nLeft, int nTop, int nWidth, int nHeight )
{
    m_nLeft = nLeft;
    m_nTop = nTop;
    m_nWidth = ( nWidth > 0 ) ? nWidth : 0;
    m_nHeight = ( nHeight > 0 ) ? nHeight : 0;
    m_nGridX = 2 * nLeft;
    m_nGridY = 2 * nTop;

    if ( ( m_nWidth == 0 ) || ( m_nHeight == 0 ) )
    {
        m_nWidth = m_nHeight = 0;
        return 0;
    }

    m_oSums.Begin( m_nLeft, m_nTop, m_nWidth, m_nHeight, PIXEL_SAMPLES );

        // Every corner, plus every center.
    return ( (long long)( m_nWidth + 1 ) * (long long)( m_nHeight + 1 ) ) +
           ( (long long)m_nWidth * (long long)m_nHeight );
}

BOOL CCornerSampler::NextRay( CScene &oScene, CRay &oRay )
{
    if ( ( m_nWidth == 0 ) || ( m_nGridY > 2 * ( m_nTop + m_nHeight ) ) )
        return FALSE;

    oRay.CreateRayAt( oScene, m_nGridX / 2.0, m_nGridY / 2.0, m_nGridX, m_nGridY );

        // Across the tile, corner rows (even) run 2 * left, ... 2 * right;
        // center rows (odd) run 2 * left + 1, ... 2 * right - 1.
    m_nGridX += 2;
    if ( m_nGridX > 2 * ( m_nLeft + m_nWidth ) )
    {
        m_nGridY++;
        m_nGridX = ( 2 * m_nLeft ) + ( ( m_nGridY & 1 ) ? 1 : 0 );
    }
    return TRUE;
}

void CCornerSampler::PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor )
{
    if ( ( nTagX < 0 ) || ( nTagY < 0 ) )
        return;

    if ( ( nTagX & 1 ) && ( nTagY & 1 ) )
        m_oSums.Add( oWriter, nTagX / 2, nTagY / 2, oColor, 1 );       // a center
    else if ( ! ( nTagX & 1 ) && ! ( nTagY & 1 ) )
    {
            // A corner, shared by up to four pixels.
        int nCornerX = nTagX / 2;
        int nCornerY = nTagY / 2;

        m_oSums.Add( oWriter, nCornerX - 1, nCornerY - 1, oColor, 1 );
        m_oSums.Add( oWriter, nCornerX,     nCornerY - 1, oColor, 1 );
        m_oSums.Add( oWriter, nCornerX - 1, nCornerY,     oColor, 1 );
        m_oSums.Add( oWriter, nCornerX,     nCornerY,     oColor, 1 );
    }
}

void CCornerSampler::End( void )
{
    m_oSums.End();
}
