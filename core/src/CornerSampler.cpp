#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "CornerSampler.h"
#include "Rays.h"
#include "Color.h"
#include "PixelWriter.h"

    // Fixed-point scale of the sums: a color channel of 1.0.
#define SAMPLE_SCALE       65536.0

    // The brightest channel a sample can add, so the sums can't overflow.
#define SAMPLE_MAXIMUM     65536.0

    // The rays averaged into each pixel.
#define PIXEL_SAMPLES      5

CCornerSampler::CCornerSampler( void ) :
    m_nWidth( 0 ),
    m_nHeight( 0 ),
    m_nGridX( 0 ),
    m_nGridY( 0 )
{
}

long long CCornerSampler::Begin( int nWidth, int nHeight )
{
    m_nWidth = ( nWidth > 0 ) ? nWidth : 0;
    m_nHeight = ( nHeight > 0 ) ? nHeight : 0;
    m_nGridX = 0;
    m_nGridY = 0;

    if ( ( m_nWidth == 0 ) || ( m_nHeight == 0 ) )
    {
        m_nWidth = m_nHeight = 0;
        m_aoSums.clear();
        return 0;
    }

    m_aoSums.assign( (size_t)m_nWidth * (size_t)m_nHeight, SPixelSum() );

        // Every corner, plus every center.
    return ( (long long)( m_nWidth + 1 ) * (long long)( m_nHeight + 1 ) ) +
           ( (long long)m_nWidth * (long long)m_nHeight );
}

BOOL CCornerSampler::NextRay( CScene &oScene, CRay &oRay )
{
    if ( ( m_nWidth == 0 ) || ( m_nGridY > 2 * m_nHeight ) )
        return FALSE;

    oRay.CreateRayAt( oScene, m_nGridX / 2.0, m_nGridY / 2.0, m_nGridX, m_nGridY );

        // Corner rows (even) run 0, 2, ... 2 * width; center rows (odd)
        // run 1, 3, ... 2 * width - 1.
    m_nGridX += 2;
    if ( m_nGridX > 2 * m_nWidth )
    {
        m_nGridY++;
        m_nGridX = ( m_nGridY & 1 ) ? 1 : 0;
    }
    return TRUE;
}

void CCornerSampler::PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor )
{
    CColor      oSample( oColor );
    FLT         anChannel[ 3 ] = { oSample.GetR(), oSample.GetG(), oSample.GetB() };
    uint64_t    anColor[ 3 ];

    if ( ( nTagX < 0 ) || ( nTagY < 0 ) )
        return;

    for ( int nCount = 0; nCount < 3; nCount++ )
    {
        FLT nValue = anChannel[ nCount ];

        if ( ! ( nValue > 0.0 ) )
            nValue = 0.0;
        else if ( nValue > SAMPLE_MAXIMUM )
            nValue = SAMPLE_MAXIMUM;
        anColor[ nCount ] = (uint64_t)( ( nValue * SAMPLE_SCALE ) + 0.5 );
    }

    if ( ( nTagX & 1 ) && ( nTagY & 1 ) )
        AddToPixel( oWriter, nTagX / 2, nTagY / 2, anColor );        // a center
    else if ( ! ( nTagX & 1 ) && ! ( nTagY & 1 ) )
    {
            // A corner, shared by up to four pixels.
        int nCornerX = nTagX / 2;
        int nCornerY = nTagY / 2;

        AddToPixel( oWriter, nCornerX - 1, nCornerY - 1, anColor );
        AddToPixel( oWriter, nCornerX,     nCornerY - 1, anColor );
        AddToPixel( oWriter, nCornerX - 1, nCornerY,     anColor );
        AddToPixel( oWriter, nCornerX,     nCornerY,     anColor );
    }
}

void CCornerSampler::AddToPixel( CPixelWriter &oWriter, int nX, int nY, const uint64_t anColor[ 3 ] )
{
    if ( ( nX < 0 ) || ( nY < 0 ) || ( nX >= m_nWidth ) || ( nY >= m_nHeight ) )
        return;

    SPixelSum &oSum = m_aoSums[ ( (size_t)nY * (size_t)m_nWidth ) + (size_t)nX ];

    oSum.nRed += anColor[ 0 ];
    oSum.nGreen += anColor[ 1 ];
    oSum.nBlue += anColor[ 2 ];
    oSum.nSamples++;

    if ( oSum.nSamples == PIXEL_SAMPLES )
    {
        const FLT nScale = (FLT)( SAMPLE_SCALE * PIXEL_SAMPLES );

        oWriter.PutPixel( nX, nY, CColor( (FLT)oSum.nRed / nScale, (FLT)oSum.nGreen / nScale, (FLT)oSum.nBlue / nScale ) );
        oSum.nRed = oSum.nGreen = oSum.nBlue = oSum.nSamples = 0;
    }
}

void CCornerSampler::End( void )
{
    m_aoSums.clear();
    m_aoSums.shrink_to_fit();
}
