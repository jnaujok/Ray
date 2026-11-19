#ifdef _WINDOWS
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Color.h"
#include "ImageFile.h"

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

    setvbuf( m_pFile, NULL, _IOFBF, 16384 );
    if ( m_pFile )
    {
        if ( bRead )
        {
            fread( &nHigh, sizeof( BYTE ), 1, m_pFile );
            fread( &nLow, sizeof( BYTE ), 1, m_pFile );
            m_nWidth = (DWORD)nHigh * 256 + (DWORD)nLow;
            fread( &nHigh, sizeof( BYTE ), 1, m_pFile );
            fread( &nLow, sizeof( BYTE ), 1, m_pFile );
            m_nHeight = (DWORD)nHigh * 256 + (DWORD)nLow;
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

    if ( m_pFile )
    {
        dwOffset = 4 + ( ( ( m_nWidth * nY ) + nX ) * 3 );

        fseek( m_pFile, dwOffset, SEEK_SET );

        fread( &nRed, sizeof( BYTE ), 1, m_pFile );
        fread( &nGreen, sizeof( BYTE ), 1, m_pFile );
        fread( &nBlue, sizeof( BYTE ), 1, m_pFile );
        
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

BOOL CImageFile::PutPoint( int nX, int nY, CColor &oColor )
{
    long    dwOffset;
    BOOL    bRetVal = FALSE;
    BYTE    nRed, nGreen, nBlue;
    CColor  oColor2 = oColor;

    oColor2.Clip();

    if ( m_pFile )
    {
        dwOffset = 4 + ( ( ( m_nWidth * nY ) + nX ) * 3 );

        fseek( m_pFile, dwOffset, SEEK_SET );

        nRed = (BYTE)(oColor2.GetR() * 255);
        nGreen = (BYTE)(oColor2.GetG() * 255);
        nBlue = (BYTE)(oColor2.GetB() * 255);

        if ( ( fwrite( &nRed, sizeof( BYTE ), 1, m_pFile ) == 1 ) &&
             ( fwrite( &nGreen, sizeof( BYTE ), 1, m_pFile ) == 1 ) &&
             ( fwrite( &nBlue, sizeof( BYTE ), 1, m_pFile ) == 1 ) )
            bRetVal = TRUE;
    }

    return bRetVal;
}
