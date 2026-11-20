#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "PixelWriter.h"
#include "Color.h"
#include "ColorModel.h"
#include "ImageFile.h"

CPixelWriter::CPixelWriter( CImageFile &oImageFile, const CColorModel &oColorModel, FLT nExposure ) :
    m_oImageFile( oImageFile ),
    m_oColorModel( oColorModel ),
    m_nExposure( nExposure )
{
}

BOOL CPixelWriter::PutPixel( int nX, int nY, const CColor &oColor )
{
    BYTE    nRed, nGreen, nBlue;

    if ( m_nExposure == 1.0 )
        m_oColorModel.ToRGB( oColor, nRed, nGreen, nBlue );
    else
    {
        CColor oScaled( oColor );
        m_oColorModel.ToRGB( oScaled * m_nExposure, nRed, nGreen, nBlue );
    }
    return m_oImageFile.PutRGB( nX, nY, nRed, nGreen, nBlue );
}
