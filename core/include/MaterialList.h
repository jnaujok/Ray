#ifndef __MATERIALLIST_H__
#define __MATERIALLIST_H__

#include "Material.h"
#include "DiffuseMaterial.h"
#include "Templates/TArray.h"

namespace IASLib
{
    class CArrayNode;
}

    // The materials an object is made of, each with its influence. The list
    // does not own the materials: copies share them.
class CMaterialList
{
    protected:
        IASLib::TArray<CMaterial *> m_apoItems;
        IASLib::TArray<FLT>         m_anInfluence;

    public:
        explicit        CMaterialList( int nSize = 1 );
        virtual        ~CMaterialList( void ) {}

        int             Add( CMaterial *pObject, FLT nInfluence );
        BOOL            Delete( int nIndex );
        int             Insert( CMaterial *pObject, FLT nInfluence, int nIndex );
        CMaterial      *GetAt( int nIndex );
        CMaterial      *operator[] ( int nIndex ) { return GetAt( nIndex ); }
        void            RemoveAll( void ) { m_apoItems.Clear(); m_anInfluence.Clear(); }
        int             GetSize( void ) const { return (int)m_apoItems.Length(); }
        FLT             GetInfluence( int nIndex );
        void            SetInfluence( int nIndex, FLT nInfluence );

            // The materials as a JSON array of objects, each with its "type"
            // and "influence".
        void            ToJson( IASLib::CArrayNode &oArray );
            // Reads the array oNode[ strKey ], if it is there. The materials it
            // replaces are deleted (the object owning the list owns them).
        BOOL            FromJson( const IASLib::CJsonNode &oNode, const char *strKey );

        friend CRayStream &operator >>( CRayStream &oStream, CMaterialList &oList );
        friend CRayStream &operator <<( CRayStream &oStream, CMaterialList &oList );
};

#endif // __MATERIALLIST_H__
