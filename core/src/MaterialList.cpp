#include "Globals.h"

#include "Globals.h"
#include "MaterialList.h"
#include "SceneJson.h"
#include "Presets.h"

#include <algorithm>

#define MATERIAL_LIST_VERSION       ((DWORD)1)

CMaterialList::CMaterialList( int nSize )
{
    if ( nSize > 0 )
    {
        m_apoItems.Reserve( (size_t)nSize );
        m_anInfluence.Reserve( (size_t)nSize );
    }
}

int CMaterialList::Add( CMaterial *pObject, FLT nInfluence )
{
    m_anInfluence.Push( nInfluence );
    m_apInfluenceMap.push_back( std::shared_ptr<const CScalarMap>() );
    return (int)m_apoItems.Push( pObject );
}

BOOL CMaterialList::Delete( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return FALSE;
    m_aoPresets.clear();
    m_apoItems.Remove( (size_t)nIndex );
    m_anInfluence.Remove( (size_t)nIndex );
    if ( (size_t)nIndex < m_apInfluenceMap.size() )
        m_apInfluenceMap.erase( m_apInfluenceMap.begin() + nIndex );
    return TRUE;
}

int CMaterialList::Insert( CMaterial *pObject, FLT nInfluence, int nIndex )
{
    if ( nIndex < 0 )
        nIndex = 0;
    if ( nIndex > GetSize() )
        nIndex = GetSize();
    m_aoPresets.clear();
    m_anInfluence.Insert( (size_t)nIndex, nInfluence );
    m_apInfluenceMap.insert( m_apInfluenceMap.begin() + (std::min)( (size_t)nIndex, m_apInfluenceMap.size() ),
                             std::shared_ptr<const CScalarMap>() );
    return (int)m_apoItems.Insert( (size_t)nIndex, pObject );
}

CMaterial *CMaterialList::GetAt( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return NULL;
    return m_apoItems[ (size_t)nIndex ];
}

FLT CMaterialList::GetInfluence( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return 0.0;
    return m_anInfluence[ (size_t)nIndex ];
}

void CMaterialList::SetInfluence( int nIndex, FLT nInfluence )
{
    if ( ( nIndex >= 0 ) && ( nIndex < GetSize() ) )
    {
        m_anInfluence[ (size_t)nIndex ] = nInfluence;
        if ( (size_t)nIndex < m_apInfluenceMap.size() )
            m_apInfluenceMap[ (size_t)nIndex ].reset();
    }
}

void CMaterialList::SetInfluenceMap( int nIndex, const CScalarMap &oMap )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) || (size_t)nIndex >= m_apInfluenceMap.size() )
        return;
    m_anInfluence[ (size_t)nIndex ] = oMap.Get();
    if ( oMap.IsMapped() )
        m_apInfluenceMap[ (size_t)nIndex ].reset( new CScalarMap( oMap ) );
    else
        m_apInfluenceMap[ (size_t)nIndex ].reset();
}

BOOL CMaterialList::IsInfluenceMapped( int nIndex ) const
{
    return ( nIndex >= 0 && (size_t)nIndex < m_apInfluenceMap.size() && m_apInfluenceMap[ (size_t)nIndex ] ) ? TRUE : FALSE;
}

FLT CMaterialList::InfluenceAt( int nIndex, CMapPoint &oAt )
{
    if ( IsInfluenceMapped( nIndex ) )
        return m_apInfluenceMap[ (size_t)nIndex ]->At( oAt );
    return GetInfluence( nIndex );
}

void CMaterialList::WriteMaterial( IASLib::CArrayNode &oArray, int nIndex )
{
    CDynMaterialList     oMats;
    CMaterial           *pMaterial = GetAt( nIndex );
    IASLib::CObjectNode *pNode = oArray.addObject();
    LPCSTR               strType = oMats.JsonTypeOf( pMaterial->ClassType() );

    pNode->put( "type", strType ? strType : pMaterial->ClassType() );
    if ( IsInfluenceMapped( nIndex ) )
        m_apInfluenceMap[ (size_t)nIndex ]->Write( *pNode, "influence" );
    else
        RayJson::PutNumber( *pNode, "influence", GetInfluence( nIndex ) );
    pMaterial->ToJson( *pNode );
    pMaterial->TextureToJson( *pNode );
}

BOOL CMaterialList::PresetMatches( const SPresetEntry &oEntry )
{
    if ( oEntry.nFirst < 0 || oEntry.nCount <= 0 || oEntry.nFirst + oEntry.nCount > GetSize() )
        return FALSE;

    IASLib::CObjectNode oHolder;
    CMaterialList       oFresh;
    IASLib::CArrayNode  oMine;
    IASLib::CArrayNode  oTheirs;
    BOOL                bSame;

    oHolder.putArray( "entry" )->add( oEntry.pEntry->clone() );
    if ( ! oFresh.FromJson( oHolder, "entry" ) )
        return FALSE;
    for ( int nIndex = 0; nIndex < oEntry.nCount; nIndex++ )
        WriteMaterial( oMine, oEntry.nFirst + nIndex );
    oFresh.ToJsonInFull( oTheirs );
    bSame = ( RayJson::Write( oMine ) == RayJson::Write( oTheirs ) );
    for ( int nIndex = 0; nIndex < oFresh.GetSize(); nIndex++ )
        delete oFresh.GetAt( nIndex );
    return bSame;
}

void CMaterialList::ToJson( IASLib::CArrayNode &oArray )
{
    int nIndex = 0;

    while ( nIndex < GetSize() )
    {
        const SPresetEntry *pEntry = NULL;

        for ( size_t nPreset = 0; nPreset < m_aoPresets.size() && pEntry == NULL; nPreset++ )
        {
            if ( m_aoPresets[ nPreset ].nFirst == nIndex )
                pEntry = &m_aoPresets[ nPreset ];
        }
        if ( pEntry && PresetMatches( *pEntry ) )
        {
            oArray.add( pEntry->pEntry->clone() );
            nIndex += pEntry->nCount;
            continue;
        }
        WriteMaterial( oArray, nIndex );
        nIndex++;
    }
}

void CMaterialList::ToJsonInFull( IASLib::CArrayNode &oArray )
{
    for ( int nIndex = 0; nIndex < GetSize(); nIndex++ )
        WriteMaterial( oArray, nIndex );
}

BOOL CMaterialList::ReadMaterial( const IASLib::CJsonNode &oMaterial )
{
    CDynMaterialList    oMats;
    std::string         strType;
    CScalarMap          oInfluence( 1.0 );

    if ( ! RayJson::GetString( oMaterial, "type", strType, TRUE ) ||
         ! oInfluence.Read( oMaterial, "influence", FALSE ) )
        return FALSE;

    CMaterial *pMaterial = oMats.Create( strType.c_str() );

    if ( pMaterial == NULL )
        return RayJson::Fail( "unknown material type \"" + strType + "\"" );
    Add( pMaterial, oInfluence.Get() );
    if ( oInfluence.IsMapped() )
        SetInfluenceMap( GetSize() - 1, oInfluence );
    return pMaterial->FromJson( oMaterial ) && pMaterial->TextureFromJson( oMaterial );
}

BOOL CMaterialList::FromJson( const IASLib::CJsonNode &oNode, const char *strKey )
{
    const IASLib::CJsonNode    *pArray = RayJson::Field( oNode, strKey, FALSE );
    CMaterialList               oRead;
    BOOL                        bRetVal = TRUE;

    if ( pArray == NULL )
        return oNode.isObject();

    RayJson::CWhere oWhere( strKey );

    if ( ! pArray->isArray() )
        return RayJson::Fail( "must be an array of materials" );

    for ( int nIndex = 0; bRetVal && nIndex < pArray->size(); nIndex++ )
    {
        RayJson::CWhere             oElement( "", nIndex );
        const IASLib::CJsonNode    &oMaterial = *pArray->get( nIndex );

        if ( oMaterial.isObject() && oMaterial.get( "preset" ) )
        {
                // A preset: the materials it stands for, remembered as one
                // entry so it can be saved the same way.
            std::unique_ptr<IASLib::CArrayNode> pExpanded( RayPresets::ExpandMaterials( oMaterial ) );
            SPresetEntry                        oEntry;

            if ( ! pExpanded )
            {
                bRetVal = FALSE;
                break;
            }
            oEntry.nFirst = oRead.GetSize();
            oEntry.nCount = pExpanded->size();
            oEntry.pEntry.reset( oMaterial.clone() );
            for ( int nPart = 0; bRetVal && nPart < pExpanded->size(); nPart++ )
                bRetVal = oRead.ReadMaterial( *pExpanded->get( nPart ) );
            oRead.m_aoPresets.push_back( oEntry );
            continue;
        }
        bRetVal = oRead.ReadMaterial( oMaterial );
    }

        // Whichever list is being dropped, its materials are deleted.
    CMaterialList &oDrop = bRetVal ? *this : oRead;

    for ( int nIndex = 0; nIndex < oDrop.GetSize(); nIndex++ )
        delete oDrop.GetAt( nIndex );
    oDrop.RemoveAll();

    if ( bRetVal )
    {
        int nBase = GetSize();

        for ( int nIndex = 0; nIndex < oRead.GetSize(); nIndex++ )
        {
            Add( oRead.GetAt( nIndex ), oRead.GetInfluence( nIndex ) );
            if ( oRead.IsInfluenceMapped( nIndex ) )
                m_apInfluenceMap.back() = oRead.m_apInfluenceMap[ (size_t)nIndex ];
        }
        for ( size_t nPreset = 0; nPreset < oRead.m_aoPresets.size(); nPreset++ )
        {
            SPresetEntry oEntry( oRead.m_aoPresets[ nPreset ] );

            oEntry.nFirst += nBase;
            m_aoPresets.push_back( oEntry );
        }
    }
    return bRetVal;
}
