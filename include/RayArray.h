#ifndef __RAYARRAY_H__
#define __RAYARRAY_H__

class CRayArray
{
    protected:
        CRay           *m_aoItems;
        int             m_nItems;
        int             m_nSize;
        CRay            m_oEmpty;
    public:
                        CRayArray( int nSize = 16 );
        virtual        ~CRayArray( void );

        int             Add( CRay &oRay );
        BOOL            Delete( int nIndex );
        int             Insert( CRay &oRay, int nIndex );
        CRay           &GetAt( int nIndex );
        CRay           &operator[] ( int nIndex );
        void            RemoveAll( void );
        int             GetSize( void ) { return m_nItems; }

        friend CStream  &operator >>( CStream &oStream, CRayArray &oRay );
        friend CStream  &operator <<( CStream &oStream, CRayArray &oRay );

    private:
        void            Resize( void );
        void            Trim( void );
};

#endif // __RAYARRAY_H__