#ifndef __COLORARRAY_H__
#define __COLORARRAY_H__

#include "Color.h"
#include "Templates/TArray.h"

class CRayStream;

class CColorArray
{
    protected:
        IASLib::TArray<CColor>  m_aoItems;
        IASLib::TArray<int>     m_anXCoords;
        IASLib::TArray<int>     m_anYCoords;
        CColor                  m_oBlack;

    public:
        explicit        CColorArray( int nSize = 1 );
        virtual        ~CColorArray( void ) {}

        int             Add( const CColor &oColor, int nX, int nY );
        BOOL            Delete( int nIndex );
        int             Insert( const CColor &oColor, int nX, int nY, int nIndex );
        CColor         &GetAt( int nIndex );
        CColor         &operator[] ( int nIndex ) { return GetAt( nIndex ); }
        void            RemoveAll( void );
        int             GetSize( void ) const { return (int)m_aoItems.Length(); }
        int             GetX( int nIndex );
        void            SetX( int nIndex, int nX );
        int             GetY( int nIndex );
        void            SetY( int nIndex, int nY );

        friend CRayStream &operator >>( CRayStream &oStream, CColorArray &oList );
        friend CRayStream &operator <<( CRayStream &oStream, CColorArray &oList );
};

#endif // __COLORARRAY_H__
