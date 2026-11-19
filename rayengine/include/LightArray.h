#ifndef __LIGHTARRAY_H__
#define __LIGHTARRAY_H__

#include "Light.h"

class CLightArray
{
    protected:
        CLight        **m_apoItems;
        int             m_nItems;
        int             m_nSize;
    public:
                        CLightArray( int nSize = 16 );
        virtual        ~CLightArray( void );

        int             Add( CLight *pObject );
        BOOL            Delete( int nIndex );
        int             Insert( CLight *pObject, int nIndex );
        CLight         *GetAt( int nIndex );
        CLight         *operator[] ( int nIndex );
        void            RemoveAll( void );
        int             GetSize( void ) { return m_nItems; }
    private:
        void            Resize( void );
        void            Trim( void );
};

#endif // __LIGHTARRAY_H__