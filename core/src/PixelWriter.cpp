#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "PixelWriter.h"
#include "ColorModel.h"
#include "ImageFile.h"

CPixelWriter::CPixelWriter( CImageFile &oImageFile, const CColorModel &oColorModel ) :
    m_oImageFile( oImageFile ),
    m_oColorModel( oColorModel )
{
}

BOOL CPixelWriter::PutPixel( int nX, int nY, const CColor &oColor )
{
    BYTE    nRed, nGreen, nBlue;

    m_oColorModel.ToRGB( oColor, nRed, nGreen, nBlue );
    return m_oImageFile.PutRGB( nX, nY, nRed, nGreen, nBlue );
}
