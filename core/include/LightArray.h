#ifndef __LIGHTARRAY_H__
#define __LIGHTARRAY_H__

#include "Templates/TArray.h"

class CLight;

    // An array of CLight pointers. RemoveAll() deletes the objects; Delete()
    // only takes one out of the array.
class CLightArray
{
    protected:
        IASLib::TArray<CLight *>  m_apoItems;

    public:
        explicit        CLightArray( int nSize = 16 );
        virtual        ~CLightArray( void ) {}

        int             Add( CLight *pObject );
        BOOL            Delete( int nIndex );
        int             Insert( CLight *pObject, int nIndex );
        CLight       *GetAt( int nIndex );
        CLight       *operator[] ( int nIndex ) { return GetAt( nIndex ); }
        void            RemoveAll( void );
        int             GetSize( void ) const { return (int)m_apoItems.Length(); }

    private:
                        CLightArray( const CLightArray & );
        CLightArray  &operator =( const CLightArray & );
};

#endif
