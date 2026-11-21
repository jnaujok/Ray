#ifndef __MATERIALLIST_H__
#define __MATERIALLIST_H__

#include "Material.h"
#include "DiffuseMaterial.h"
#include "Templates/TArray.h"

#include <memory>
#include <vector>

namespace IASLib
{
    class CArrayNode;
}

    // The materials an object is made of, each with its influence. The list
    // does not own the materials: copies share them.
    //
    // An entry of a scene's "materials" array can name a preset (see
    // Presets.h), which stands for several materials. The list remembers the
    // entry and which materials it gave, and writes the entry back in their
    // place while they still match it.
class CMaterialList
{
    protected:
        IASLib::TArray<CMaterial *> m_apoItems;
        IASLib::TArray<FLT>         m_anInfluence;

        struct SPresetEntry
        {
            int                                         nFirst;     // the first material it gave
            int                                         nCount;     // how many
            std::shared_ptr<const IASLib::CJsonNode>    pEntry;     // the entry as the scene gave it
        };
        std::vector<SPresetEntry>   m_aoPresets;

            // Writes material nIndex into oArray.
        void            WriteMaterial( IASLib::CArrayNode &oArray, int nIndex );
            // Whether a preset entry still gives exactly the materials it did.
        BOOL            PresetMatches( const SPresetEntry &oEntry );
            // Reads one material entry (not a preset) into this list.
        BOOL            ReadMaterial( const IASLib::CJsonNode &oMaterial );

    public:
        explicit        CMaterialList( int nSize = 1 );
        virtual        ~CMaterialList( void ) {}

        int             Add( CMaterial *pObject, FLT nInfluence );
        BOOL            Delete( int nIndex );
        int             Insert( CMaterial *pObject, FLT nInfluence, int nIndex );
        CMaterial      *GetAt( int nIndex );
        CMaterial      *operator[] ( int nIndex ) { return GetAt( nIndex ); }
        void            RemoveAll( void ) { m_apoItems.Clear(); m_anInfluence.Clear(); m_aoPresets.clear(); }
        int             GetSize( void ) const { return (int)m_apoItems.Length(); }
        FLT             GetInfluence( int nIndex );
        void            SetInfluence( int nIndex, FLT nInfluence );

            // The materials as a JSON array of objects, each with its "type"
            // and "influence".
        void            ToJson( IASLib::CArrayNode &oArray );
            // The same, with presets written out in full.
        void            ToJsonInFull( IASLib::CArrayNode &oArray );
            // Reads the array oNode[ strKey ], if it is there. The materials it
            // replaces are deleted (the object owning the list owns them).
        BOOL            FromJson( const IASLib::CJsonNode &oNode, const char *strKey );

};

#endif // __MATERIALLIST_H__
