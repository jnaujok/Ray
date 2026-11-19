#ifndef __IMAGE_FILE__
#define __IMAGE_FILE__

#include "Globals.h"
#include <stdio.h>

class CColor;

class CImageFile
{
    protected:
        DWORD       m_nWidth;
        DWORD       m_nHeight;
        FILE       *m_pFile;
    public:
                    CImageFile( LPCSTR strFilename, int nWidth, int nHeight, BOOL bRead );
        virtual    ~CImageFile( void );

        CColor      GetPoint( int nX, int nY );
        BOOL        PutPoint( int nX, int nY, CColor &oColor );

        DWORD       GetWidth( void ) { return m_nWidth; }
        DWORD       GetHeight( void ) { return m_nHeight; }
};

#endif
