#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "RayStream.h"
#include "MaterialList.h"
#include "SceneJson.h"

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
    return (int)m_apoItems.Push( pObject );
}

BOOL CMaterialList::Delete( int nIndex )
{
    if ( ( nIndex < 0 ) || ( nIndex >= GetSize() ) )
        return FALSE;
    m_apoItems.Remove( (size_t)nIndex );
    m_anInfluence.Remove( (size_t)nIndex );
    return TRUE;
}

int CMaterialList::Insert( CMaterial *pObject, FLT nInfluence, int nIndex )
{
    if ( nIndex < 0 )
        nIndex = 0;
    if ( nIndex > GetSize() )
        nIndex = GetSize();
    m_anInfluence.Insert( (size_t)nIndex, nInfluence );
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
        m_anInfluence[ (size_t)nIndex ] = nInfluence;
}

CRayStream &operator >>( CRayStream &oStream, CMaterialList &oList )
{
    DWORD               dwVersion;
    DWORD               dwCount;
    CMaterial          *pMaterial;
    IASLib::CString             strMaterial;
    CDynMaterialList    oMats;
    FLT                 nInfluence;

    oStream >> dwVersion;
    switch ( dwVersion )
    {
        case MATERIAL_LIST_VERSION:
            oList.RemoveAll();
            oStream >> dwCount;
            while ( dwCount )
            {
                oStream >> strMaterial;
                pMaterial = oMats.Instantiate( strMaterial, oStream );
                oStream >> nInfluence;
                oList.Add( pMaterial, nInfluence );
                dwCount--;
            }
            break;

        default:
            break;
    }
    return oStream;
}

CRayStream &operator <<( CRayStream &oStream, CMaterialList &oList )
{
    CMaterial          *pMaterial;
    IASLib::CString             strMaterial;
    CDynMaterialList    oMats;
    FLT                 nInfluence;
    int                 nCount;

    oStream << MATERIAL_LIST_VERSION;

    oStream << (DWORD)oList.GetSize();
    nCount = 0;
    while ( nCount < oList.GetSize() )
    {
        pMaterial = oList[ nCount ];
        nInfluence = oList.GetInfluence( nCount );
        strMaterial = pMaterial->ClassType();
        oStream << strMaterial;
        pMaterial->Out( oStream );
        oStream << nInfluence;
        nCount++;
    }
    return oStream;
}

void CMaterialList::ToJson( IASLib::CArrayNode &oArray )
{
    CDynMaterialList    oMats;

    for ( int nIndex = 0; nIndex < GetSize(); nIndex++ )
    {
        CMaterial           *pMaterial = GetAt( nIndex );
        IASLib::CObjectNode *pNode = oArray.addObject();
        LPCSTR               strType = oMats.JsonTypeOf( pMaterial->ClassType() );

        pNode->put( "type", strType ? strType : pMaterial->ClassType() );
        RayJson::PutNumber( *pNode, "influence", GetInfluence( nIndex ) );
        pMaterial->ToJson( *pNode );
    }
}

BOOL CMaterialList::FromJson( const IASLib::CJsonNode &oNode, const char *strKey )
{
    const IASLib::CJsonNode    *pArray = RayJson::Field( oNode, strKey, FALSE );
    CDynMaterialList            oMats;
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
        std::string                 strType;
        FLT                         nInfluence = 1.0;

        if ( ! RayJson::GetString( oMaterial, "type", strType, TRUE ) ||
             ! RayJson::GetNumber( oMaterial, "influence", nInfluence, FALSE ) )
        {
            bRetVal = FALSE;
            break;
        }

        CMaterial *pMaterial = oMats.Create( strType.c_str() );

        if ( pMaterial == NULL )
            bRetVal = RayJson::Fail( "unknown material type \"" + strType + "\"" );
        else
        {
            oRead.Add( pMaterial, nInfluence );
            bRetVal = pMaterial->FromJson( oMaterial );
        }
    }

        // Whichever list is being dropped, its materials are deleted.
    CMaterialList &oDrop = bRetVal ? *this : oRead;

    for ( int nIndex = 0; nIndex < oDrop.GetSize(); nIndex++ )
        delete oDrop.GetAt( nIndex );
    oDrop.RemoveAll();

    if ( bRetVal )
    {
        for ( int nIndex = 0; nIndex < oRead.GetSize(); nIndex++ )
            Add( oRead.GetAt( nIndex ), oRead.GetInfluence( nIndex ) );
    }
    return bRetVal;
}
