#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "PixelAccumulator.h"
#include "Color.h"
#include "PixelWriter.h"

    // Fixed-point scale of the sums: a color channel of 1.0.
#define SAMPLE_SCALE       65536.0

    // The brightest channel a sample can add, so the sums can't overflow.
#define SAMPLE_MAXIMUM     65536.0

CPixelAccumulator::CPixelAccumulator( void ) :
    m_nWidth( 0 ),
    m_nHeight( 0 ),
    m_nTotalWeight( 1 )
{
}

void CPixelAccumulator::Begin( int nWidth, int nHeight, uint32_t nTotalWeight )
{
    m_nWidth = ( nWidth > 0 ) ? nWidth : 0;
    m_nHeight = ( nHeight > 0 ) ? nHeight : 0;
    m_nTotalWeight = ( nTotalWeight > 0 ) ? nTotalWeight : 1;
    m_aoSums.assign( (size_t)m_nWidth * (size_t)m_nHeight, SPixelSum() );
}

void CPixelAccumulator::Add( CPixelWriter &oWriter, int nX, int nY, const CColor &oColor, uint32_t nWeight )
{
    if ( ( nX < 0 ) || ( nY < 0 ) || ( nX >= m_nWidth ) || ( nY >= m_nHeight ) )
        return;

    CColor      oSample( oColor );
    FLT         anChannel[ 3 ] = { oSample.GetR(), oSample.GetG(), oSample.GetB() };
    uint64_t    anColor[ 3 ];

    for ( int nCount = 0; nCount < 3; nCount++ )
    {
        FLT nValue = anChannel[ nCount ];

        if ( ! ( nValue > 0.0 ) )
            nValue = 0.0;
        else if ( nValue > SAMPLE_MAXIMUM )
            nValue = SAMPLE_MAXIMUM;
        anColor[ nCount ] = (uint64_t)( ( nValue * SAMPLE_SCALE ) + 0.5 );
    }

    SPixelSum &oSum = m_aoSums[ ( (size_t)nY * (size_t)m_nWidth ) + (size_t)nX ];

    oSum.nRed += anColor[ 0 ] * nWeight;
    oSum.nGreen += anColor[ 1 ] * nWeight;
    oSum.nBlue += anColor[ 2 ] * nWeight;
    oSum.nWeight += nWeight;

    if ( oSum.nWeight >= m_nTotalWeight )
    {
        const FLT nScale = (FLT)( SAMPLE_SCALE * m_nTotalWeight );

        oWriter.PutPixel( nX, nY, CColor( (FLT)oSum.nRed / nScale, (FLT)oSum.nGreen / nScale, (FLT)oSum.nBlue / nScale ) );
        oSum.nRed = oSum.nGreen = oSum.nBlue = 0;
        oSum.nWeight = 0;
    }
}

void CPixelAccumulator::End( void )
{
    m_aoSums.clear();
    m_aoSums.shrink_to_fit();
}
