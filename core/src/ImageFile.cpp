#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Color.h"
#include "ImageFile.h"
#include "ClipColorModel.h"

CImageFile::CImageFile( LPCSTR strFilename, int nWidth, int nHeight, BOOL bRead )
{
    DWORD       nX;
    DWORD       nY;
    BYTE        nColor;
    BYTE        nLow;
    BYTE        nHigh;

    m_nWidth = (DWORD)nWidth;
    m_nHeight = (DWORD)nHeight;

    if ( bRead )
        m_pFile = fopen( strFilename, "r+b" );
    else
        m_pFile = fopen( strFilename, "w+b" );

    if ( m_pFile )
    {
        setvbuf( m_pFile, NULL, _IOFBF, 16384 );

        if ( bRead )
        {
            BYTE    achHeader[ 4 ];

            if ( fread( achHeader, sizeof( BYTE ), 4, m_pFile ) == 4 )
            {
                m_nWidth = (DWORD)achHeader[ 0 ] * 256 + (DWORD)achHeader[ 1 ];
                m_nHeight = (DWORD)achHeader[ 2 ] * 256 + (DWORD)achHeader[ 3 ];
            }
            else
            {
                m_nWidth = 0;
                m_nHeight = 0;
            }
        }
        else
        {
            nLow = (BYTE)(m_nWidth % 256);
            nHigh = (BYTE)(( m_nWidth / 256 ) % 256);
            fwrite( &nHigh, sizeof( BYTE ), 1, m_pFile );
            fwrite( &nLow, sizeof( BYTE ), 1, m_pFile );
            nLow = (BYTE)(m_nHeight % 256);
            nHigh = (BYTE)(( m_nHeight / 256 ) % 256);
            fwrite( &nHigh, sizeof( BYTE ), 1, m_pFile );
            fwrite( &nLow, sizeof( BYTE ), 1, m_pFile );

            nColor = 0;
            nY = 0;
            while ( nY < m_nHeight )
            {
                nX = 0;
                while ( nX < m_nWidth )
                {
                    fwrite( &nColor, sizeof( BYTE ), 1, m_pFile );
                    fwrite( &nColor, sizeof( BYTE ), 1, m_pFile );
                    fwrite( &nColor, sizeof( BYTE ), 1, m_pFile );
                    nX++;
                }
                nY++;
            }
        }
    }
}

CImageFile::~CImageFile( void )
{
    if ( m_pFile )
    {
        fflush( m_pFile );
        fclose( m_pFile );
    }
}

CColor CImageFile::GetPoint( int nX, int nY )
{
    long    dwOffset;
    CColor  oColor;
    BYTE    nRed, nGreen, nBlue;
    FLT     fRed, fGreen, fBlue;

    if ( ( nX < 0 ) || ( nY < 0 ) || ( (DWORD)nX >= m_nWidth ) || ( (DWORD)nY >= m_nHeight ) )
        return oColor;

    if ( m_pFile )
    {
        dwOffset = 4 + ( ( ( m_nWidth * nY ) + nX ) * 3 );

        fseek( m_pFile, dwOffset, SEEK_SET );

        BYTE    achPixel[ 3 ] = { 0, 0, 0 };

        if ( fread( achPixel, sizeof( BYTE ), 3, m_pFile ) != 3 )
            return oColor;
        nRed = achPixel[ 0 ];
        nGreen = achPixel[ 1 ];
        nBlue = achPixel[ 2 ];
        
        fRed = (FLT)nRed;
        fGreen = (FLT)nGreen;
        fBlue = (FLT)nBlue;

        fRed /= 255.0;
        fGreen /= 255.0;
        fBlue /= 255.0;

        oColor.Set( fRed, fGreen, fBlue );
    }

    return oColor;
}

BOOL CImageFile::PutPoint( int nX, int nY, const CColor &oColor )
{
    CClipColorModel oModel;
    BYTE            nRed, nGreen, nBlue;

    oModel.ToRGB( oColor, nRed, nGreen, nBlue );
    return PutRGB( nX, nY, nRed, nGreen, nBlue );
}

BOOL CImageFile::PutRGB( int nX, int nY, BYTE nRed, BYTE nGreen, BYTE nBlue )
{
    long    dwOffset;
    BOOL    bRetVal = FALSE;

    if ( ( nX < 0 ) || ( nY < 0 ) || ( (DWORD)nX >= m_nWidth ) || ( (DWORD)nY >= m_nHeight ) )
        return FALSE;

    if ( m_pFile )
    {
        dwOffset = 4 + ( ( ( m_nWidth * nY ) + nX ) * 3 );

        fseek( m_pFile, dwOffset, SEEK_SET );

        if ( ( fwrite( &nRed, sizeof( BYTE ), 1, m_pFile ) == 1 ) &&
             ( fwrite( &nGreen, sizeof( BYTE ), 1, m_pFile ) == 1 ) &&
             ( fwrite( &nBlue, sizeof( BYTE ), 1, m_pFile ) == 1 ) )
            bRetVal = TRUE;
    }

    return bRetVal;
}
