#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "ImageWriter.h"
#include "Color.h"
#include "ColorModel.h"
#include "ImageFile.h"

CImageWriter::CImageWriter( CImageFile &oImageFile, const CColorModel &oColorModel, FLT nExposure ) :
    m_oImageFile( oImageFile ),
    m_oColorModel( oColorModel ),
    m_nExposure( nExposure )
{
}

BOOL CImageWriter::PutPixel( int nX, int nY, const CColor &oColor )
{
    BYTE    nRed, nGreen, nBlue;
    CColor  oScaled( oColor );

    if ( m_nExposure != 1.0 )
        oScaled = oScaled * m_nExposure;
    m_oColorModel.ToRGB( oScaled, nRed, nGreen, nBlue );
    if ( m_oImageFile.KeepsLinear() )
        m_oImageFile.PutLinear( nX, nY, m_oColorModel.ToLinear( oScaled ) );
    return m_oImageFile.PutRGB( nX, nY, nRed, nGreen, nBlue );
}
