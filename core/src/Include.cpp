/*************************************************************************
*   CInclude                                                             *
*                                                                        *
*   See Include.h.                                                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#include "Globals.h"

#include "Include.h"
#include "Mesh.h"
#include "SceneJson.h"

#include "Resources/Resource.h"
#include "Resources/ResourceFile.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>

DYNAMIC_IMPLEMENTS( CInclude, "include" );

namespace
{
    const char   *g_strArchiveMagic = "IASRES\r\n";
    const int     g_nFolderScene = 1;
    const int     g_nResourceScene = 1;
}

CInclude::CInclude( void ) :
    m_nScale( 1.0 ),
    m_oRotation( 0.0, 0.0, 0.0 ),
    m_oPosition( 0.0, 0.0, 0.0 )
{
}

CInclude::~CInclude( void )
{
    for ( CObjects *pObject : m_apObjects )
        delete pObject;
}

void CInclude::TakeObjects( std::vector<CObjects *> &apInto )
{
    m_apIncluded.clear();
    for ( CObjects *pObject : m_apObjects )
    {
        if ( pObject->IncludedBy() == NULL )
            pObject->SetIncludedBy( this );
        apInto.push_back( pObject );
        m_apIncluded.push_back( pObject );
    }
    m_apObjects.clear();
}

BOOL CInclude::ReadObjects( const IASLib::CJsonNode &oArray )
{
    BOOL bPlaced = ( m_nScale != 1.0 || m_oRotation.Magnitude() != 0.0 || m_oPosition.Magnitude() != 0.0 ) ? TRUE : FALSE;

    if ( ! oArray.isArray() )
        return RayJson::Fail( "objects must be an array of objects" );
    for ( int nIndex = 0; nIndex < oArray.size(); nIndex++ )
    {
        RayJson::CWhere oElement( "objects", nIndex );
        CObjects       *pObject = CObjects::ReadJson( *oArray.get( nIndex ), FALSE,
            [ this, bPlaced ]( CObjects *pNew ) -> BOOL
            {
                CMesh *pMesh = dynamic_cast<CMesh *>( pNew );

                if ( ! bPlaced )
                    return TRUE;
                if ( pMesh == NULL )
                    return RayJson::Fail( "an include can only place meshes, so one with a scale, rotation or position can't bring in a " +
                                          std::string( pNew->ClassType() ) );
                pMesh->SetOuterPlacement( m_nScale, m_oRotation, m_oPosition );
                return TRUE;
            } );

        if ( pObject == NULL )
            return FALSE;

        CMesh *pMesh = dynamic_cast<CMesh *>( pObject );

        if ( pMesh && bPlaced )
            pMesh->ApplyOuterToInline();
        m_apObjects.push_back( pObject );

            // An include in the file brings its objects in too (it can't be
            // placed again: the check above stops one that would be).
        CInclude *pInner = dynamic_cast<CInclude *>( pObject );

        if ( pInner )
        {
            std::vector<CObjects *> apInner;

            pInner->TakeObjects( apInner );
            for ( CObjects *pInnerObject : apInner )
                m_apObjects.push_back( pInnerObject );
        }
    }
    return TRUE;
}

void CInclude::ToJson( IASLib::CObjectNode &oNode )
{
    oNode.put( "file", RayJson::SceneFileName( m_strFile, m_strPath, m_pFileData ) );
    if ( m_nScale != 1.0 )
        RayJson::PutNumber( oNode, "scale", m_nScale );
    if ( m_oRotation.Magnitude() != 0.0 )
        RayJson::PutVector( oNode, "rotation", m_oRotation );
    if ( m_oPosition.Magnitude() != 0.0 )
        RayJson::PutVector( oNode, "position", m_oPosition );

        // A task's copy carries the objects themselves (with their files'
        // data), so an engine needs no file; a saved scene only names the
        // file, but its objects are still written (and dropped) so the
        // files they use go into an archive with it.
    std::unique_ptr<IASLib::CObjectNode> pScratch;
    IASLib::CArrayNode                  *pObjects;

    if ( RayJson::EmbeddingData() )
        pObjects = oNode.putArray( "objects" );
    else
    {
        pScratch.reset( new IASLib::CObjectNode );
        pObjects = pScratch->putArray( "objects" );
    }
    for ( CObjects *pObject : m_apIncluded )
    {
        if ( pObject->IncludedBy() != this )
            continue;

        IASLib::CObjectNode    *pChild = pObjects->addObject();
        LPCSTR                  strType = g_pDynamicList->JsonTypeOf( pObject->ClassType() );

        pChild->put( "type", strType ? strType : pObject->ClassType() );
        if ( ! pObject->GetName().empty() )
            pChild->put( "name", pObject->GetName().c_str() );
        pObject->ToJson( *pChild );
    }
}

BOOL CInclude::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "file", "scale", "rotation", "position", "objects", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetString( oNode, "file", m_strFile, TRUE ) ||
         ! RayJson::GetNumber( oNode, "scale", m_nScale, FALSE ) ||
         ! RayJson::GetVector( oNode, "rotation", m_oRotation, FALSE ) ||
         ! RayJson::GetVector( oNode, "position", m_oPosition, FALSE ) )
        return FALSE;
    if ( ! ( m_nScale > 0.0 ) )
        return RayJson::Fail( "scale must be more than 0" );
    for ( CObjects *pObject : m_apObjects )
        delete pObject;
    m_apObjects.clear();

        // A task's copy: the objects are here.
    const IASLib::CJsonNode *pObjects = oNode.get( "objects" );

    if ( pObjects )
    {
            // They were written already placed by their own fields; the
            // include's placement goes on again as they are read.
        return ReadObjects( *pObjects );
    }

    std::string strData, strError;

    if ( ! RayJson::ReadSceneFile( m_strFile, strData, m_strPath, strError ) )
        return RayJson::Fail( "can't open the included scene \"" + m_strFile + "\"" );
    m_pFileData = std::make_shared<const std::string>( strData );

        // Files the included scene names are relative to it.
    std::string             strWhere = m_strPath.empty() ? RayJson::ResolvePath( m_strFile ) : m_strPath;
    std::string             strFolder = std::filesystem::path( strWhere ).parent_path().string();
    RayJson::CBaseDirectory oBase( strFolder );
    size_t                  nStart = 0;

    while ( nStart < strData.size() && ( strData[ nStart ] == ' ' || strData[ nStart ] == '\t' || strData[ nStart ] == '\r' ||
                                         strData[ nStart ] == '\n' ) )
        nStart++;

    std::string strJson;
    std::string strTemporary;
    std::unique_ptr<IASLib::CResourceFile>   pArchive;
    std::unique_ptr<RayJson::CArchiveSource> pSource;

    if ( nStart < strData.size() && strData[ nStart ] == '{' )
        strJson = strData.substr( nStart );
    else if ( strData.compare( 0, 8, g_strArchiveMagic, 8 ) == 0 )
    {
            // An archive: opened from the disk, or (inside another archive)
            // from a copy written out for the moment.
        std::string strOpen = m_strPath;

        if ( strOpen.empty() )
        {
            strTemporary = ( std::filesystem::temp_directory_path() /
                             ( "ray_include_" + std::to_string( (unsigned long long)(uintptr_t)this ) + ".ray" ) ).string();

            std::ofstream oOut( strTemporary, std::ios::binary );

            oOut.write( strData.data(), (std::streamsize)strData.size() );
            if ( ! oOut )
                return RayJson::Fail( "can't read the included archive \"" + m_strFile + "\"" );
            strOpen = strTemporary;
        }
        pArchive.reset( new IASLib::CResourceFile );
        if ( ! pArchive->Open( strOpen.c_str(), IASLib::CResourceFile::READ_ONLY ) )
        {
            if ( ! strTemporary.empty() )
                remove( strTemporary.c_str() );
            return RayJson::Fail( "the included archive \"" + m_strFile + "\" can't be read" );
        }

        std::shared_ptr<const IASLib::CResource> pScene = pArchive->Get( (short)g_nFolderScene, (short)g_nResourceScene );

        if ( ! pScene )
        {
            pArchive.reset();
            if ( ! strTemporary.empty() )
                remove( strTemporary.c_str() );
            return RayJson::Fail( "the included archive \"" + m_strFile + "\" has no scene in it" );
        }
        strJson.assign( (const char *)pScene->GetData(), pScene->GetLength() );
        pSource.reset( new RayJson::CArchiveSource( *pArchive ) );
    }
    else
        return RayJson::Fail( "the included file \"" + m_strFile + "\" is not a scene (a scene archive or JSON)" );

    std::unique_ptr<IASLib::CJsonNode> pRoot( RayJson::Parse( strJson ) );
    BOOL                               bRead;

    if ( ! pRoot )
        bRead = FALSE;
    else
    {
        RayJson::CWhere          oWhere( m_strFile.c_str() );
        const IASLib::CJsonNode *pArray = pRoot->isObject() ? pRoot->get( "objects" ) : NULL;

        bRead = pArray ? ReadObjects( *pArray ) : TRUE;
    }
    pSource.reset();
    pArchive.reset();
    if ( ! strTemporary.empty() )
        remove( strTemporary.c_str() );
    return bRead;
}
