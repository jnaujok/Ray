#ifndef __COLORARRAY_H__
#define __COLORARRAY_H__

#include "Color.h"

class CColorArray
{
    protected:
        CColor         *m_aoItems;
        int            *m_anXCoords;
        int            *m_anYCoords;
        int             m_nItems;
        int             m_nSize;
        CColor          m_oBlack;
    public:
                        CColorArray( int nSize = 1 );
                        CColorArray( const CColorArray &oSrc );
        virtual        ~CColorArray( void );

        int             Add( CColor &oColor, int nX, int nY );
        BOOL            Delete( int nIndex );
        int             Insert( CColor &oColor, int nX, int nY, int nIndex );
        CColor         &GetAt( int nIndex );
        CColor         &operator[] ( int nIndex );
        void            RemoveAll( void );
        int             GetSize( void ) { return m_nItems; }
        int             GetX( int nIndex );
        void            SetX( int nIndex, int nX );
        int             GetY( int nIndex );
        void            SetY( int nIndex, int nY );

        CColorArray    &operator =( const CColorArray &oSrc );

        friend CStream &operator >>( CStream &oStream, CColorArray &oList );
        friend CStream &operator <<( CStream &oStream, CColorArray &oList );
    private:
        void            Resize( void );
        void            Trim( void );
};

#endif // __COLORARRAY_H__
