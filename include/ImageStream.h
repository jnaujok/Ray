#ifndef __IMAGE_STREAM_H__
#define __IMAGE_STREAM_H__

#include <stdio.h>

class CImageStream
{
    protected:
        FILE           *m_poFile;
        int             m_nWidth;
        int             m_nHeight;
        int             m_nColorDepth;

    public:
                        CImageStream( LPCSTR strName, int nHeight, int nWidth, int nDepth );
        virtual        ~CImageStream( void );

        virtual void    Write( int nX, int nY, CColor &oColor );
        virtual CColor  Read( int nX, int nY );

        virtual int     GetHeight( void ) { return m_nHeight; }
        virtual int     GetWidth( void ) { return m_nWidth; }
        virtual int     GetDepth( void ) { return m_nColorDepth; }
};


#endif