#ifndef __IMAGE_FILE__
#define __IMAGE_FILE__

#include "Globals.h"
#include <stdio.h>
#include <vector>

class CColor;

class CImageFile
{
    protected:
        DWORD       m_nWidth;
        DWORD       m_nHeight;
        FILE       *m_pFile;
        std::vector<float>  m_anLinear;     // linear colors, if KeepLinear was called
    public:
                    CImageFile( LPCSTR strFilename, int nWidth, int nHeight, BOOL bRead );
        virtual    ~CImageFile( void );

        CColor      GetPoint( int nX, int nY );
            // Writes a color the way the 1997 code did (the clip color model).
        BOOL        PutPoint( int nX, int nY, const CColor &oColor );
        BOOL        PutRGB( int nX, int nY, BYTE nRed, BYTE nGreen, BYTE nBlue );
            // Keep every pixel's linear color as well as its bytes, for a
            // high dynamic range file.
        void        KeepLinear( void ) { m_anLinear.assign( (size_t)m_nWidth * (size_t)m_nHeight * 3, 0.0f ); }
        BOOL        KeepsLinear( void ) const { return m_anLinear.empty() ? FALSE : TRUE; }
        BOOL        PutLinear( int nX, int nY, const CColor &oColor );
            // Red, green and blue for each pixel, row by row from the top left.
        const std::vector<float> &GetLinear( void ) const { return m_anLinear; }
        BOOL        IsOpen( void ) const { return ( m_pFile != NULL ) ? TRUE : FALSE; }

        DWORD       GetWidth( void ) { return m_nWidth; }
        DWORD       GetHeight( void ) { return m_nHeight; }
};

#endif
