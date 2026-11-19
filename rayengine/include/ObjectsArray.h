#ifndef __OBJECTSARRAY_H__
#define __OBJECTSARRAY_H__

class CObjectsArray
{
    protected:
        CObjects      **m_apoItems;
        int             m_nItems;
        int             m_nSize;
    public:
                        CObjectsArray( int nSize = 16 );
        virtual        ~CObjectsArray( void );

        int             Add( CObjects *pObject );
        BOOL            Delete( int nIndex );
        int             Insert( CObjects *pObject, int nIndex );
        CObjects       *GetAt( int nIndex );
        CObjects       *operator[] ( int nIndex );
        void            RemoveAll( void );
        int             GetSize( void ) { return m_nItems; }
    private:
        void            Resize( void );
        void            Trim( void );
};

#endif // __OBJECTSARRAY_H__