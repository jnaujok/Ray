#ifndef __OBJECTSARRAY_H__
#define __OBJECTSARRAY_H__

#include "Templates/TArray.h"

class CObjects;

    // An array of CObjects pointers. RemoveAll() deletes the objects; Delete()
    // only takes one out of the array.
class CObjectsArray
{
    protected:
        IASLib::TArray<CObjects *>  m_apoItems;

    public:
        explicit        CObjectsArray( int nSize = 16 );
        virtual        ~CObjectsArray( void ) {}

        int             Add( CObjects *pObject );
        BOOL            Delete( int nIndex );
        int             Insert( CObjects *pObject, int nIndex );
        CObjects       *GetAt( int nIndex );
        CObjects       *operator[] ( int nIndex ) { return GetAt( nIndex ); }
        void            RemoveAll( void );
        int             GetSize( void ) const { return (int)m_apoItems.Length(); }

    private:
                        CObjectsArray( const CObjectsArray & );
        CObjectsArray  &operator =( const CObjectsArray & );
};

#endif
