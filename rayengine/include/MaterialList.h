#ifndef __MATERIALLIST_H__
#define __MATERIALLIST_H__

#include "Material.h"
#include "DiffuseMaterial.h"

class CMaterialList
{
    protected:
        CMaterial     **m_apoItems;
        FLT            *m_anInfluence;
        int             m_nItems;
        int             m_nSize;
    public:
                        CMaterialList( int nSize = 1 );
                        CMaterialList( const CMaterialList &oSrc );
        virtual        ~CMaterialList( void );

        int             Add( CMaterial *pObject, FLT nInfluence );
        BOOL            Delete( int nIndex );
        int             Insert( CMaterial *pObject, FLT nInfluence, int nIndex );
        CMaterial      *GetAt( int nIndex );
        CMaterial      *operator[] ( int nIndex );
        void            RemoveAll( void );
        int             GetSize( void ) { return m_nItems; }
        FLT             GetInfluence( int nIndex );
        void            SetInfluence( int nIndex, FLT nInfluence );

        CMaterialList  &operator =( const CMaterialList &oSrc );

        friend CStream &operator >>( CStream &oStream, CMaterialList &oList );
        friend CStream &operator <<( CStream &oStream, CMaterialList &oList );
    private:
        void            Resize( void );
        void            Trim( void );
};

#endif // __MATERIALLIST_H__