#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "CenterSampler.h"
#include "Rays.h"
#include "Color.h"
#include "PixelWriter.h"

CCenterSampler::CCenterSampler( void ) :
    m_nWidth( 0 ),
    m_nHeight( 0 ),
    m_nX( 0 ),
    m_nY( 0 )
{
}

long long CCenterSampler::Begin( int nWidth, int nHeight )
{
    m_nWidth = ( nWidth > 0 ) ? nWidth : 0;
    m_nHeight = ( nHeight > 0 ) ? nHeight : 0;
    m_nX = 0;
    m_nY = 0;

    return (long long)m_nWidth * (long long)m_nHeight;
}

BOOL CCenterSampler::NextRay( CScene &oScene, CRay &oRay )
{
    if ( ( m_nY >= m_nHeight ) || ( m_nWidth == 0 ) )
        return FALSE;

    oRay.CreateRay( oScene, m_nX, m_nY );

    if ( ++m_nX >= m_nWidth )
    {
        m_nX = 0;
        m_nY++;
    }
    return TRUE;
}

void CCenterSampler::PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor )
{
    oWriter.PutPixel( nTagX, nTagY, oColor );
}
