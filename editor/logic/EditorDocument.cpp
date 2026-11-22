#include "Globals.h"
#include "EditorDocument.h"

#include "SceneJson.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace RayEditor
{
    namespace
    {
            // How much memory undo may hold, in bytes of scene text; the
            // oldest steps go first. At least a few steps are always kept.
        const size_t    UNDO_BYTES = (size_t)256 * 1024 * 1024;
        const size_t    UNDO_STEPS = 100;

        const char     *NEW_SCENE =
            "{\n"
            "  \"format\": \"Ray scene\", \"version\": 3,\n"
            "  \"settings\": { \"imageWidth\": 640, \"imageHeight\": 480, \"imageFile\": \"ray.out\", \"colorDepth\": 8,\n"
            "                \"maximumDepth\": 16, \"depthThreshold\": 0.01 },\n"
            "  \"background\": [0.55, 0.7, 0.95],\n"
            "  \"camera\": { \"position\": [0.0, 3.0, -12.0], \"lookAt\": [0.0, 1.0, 0.0], \"up\": [0.0, 1.0, 0.0],\n"
            "              \"verticalAngle\": 30.0, \"sampler\": \"none\", \"colorModel\": \"clip\" },\n"
            "  \"objects\": [\n"
            "    { \"type\": \"plane\", \"position\": [0.0, 0.0, 0.0], \"normal\": [0.0, 1.0, 0.0], \"color\": [0.5, 0.5, 0.5],\n"
            "      \"materials\": [ { \"type\": \"ambient\", \"influence\": 1.0, \"color\": [0.08, 0.08, 0.08] },\n"
            "                     { \"type\": \"diffuse\", \"influence\": 0.85, \"color\": [1.0, 1.0, 1.0], \"diffuseCast\": false,\n"
            "                       \"texture\": { \"type\": \"checker\", \"colors\": [[0.15, 0.15, 0.17], [0.85, 0.85, 0.8]],\n"
            "                                    \"size\": 1.0, \"solid\": false } } ] },\n"
            "    { \"type\": \"sphere\", \"center\": [0.0, 1.0, 0.0], \"radius\": 1.0, \"color\": [0.5, 0.5, 0.5],\n"
            "      \"materials\": [ { \"type\": \"ambient\", \"influence\": 1.0, \"color\": [0.08, 0.02, 0.02] },\n"
            "                     { \"type\": \"diffuse\", \"influence\": 0.8, \"color\": [0.85, 0.2, 0.15], \"diffuseCast\": false },\n"
            "                     { \"type\": \"specular\", \"influence\": 0.4, \"color\": [1.0, 1.0, 1.0], \"power\": 0.2,\n"
            "                       \"indexOfRefraction\": 1.5, \"materialHighlight\": false } ] }\n"
            "  ],\n"
            "  \"lights\": [ { \"type\": \"point\", \"position\": [-6.0, 10.0, -8.0], \"color\": [1.0, 1.0, 1.0] } ]\n"
            "}\n";

        const char     *MATERIALS =
            "\"color\": [0.5, 0.5, 0.5], \"materials\": [ { \"type\": \"ambient\", \"influence\": 1.0, \"color\": [0.08, 0.08, 0.08] }, "
            "{ \"type\": \"diffuse\", \"influence\": 0.85, \"color\": [0.8, 0.8, 0.8], \"diffuseCast\": false } ] }";

        std::string Capitalized( const std::string &strText )
        {
            std::string strOut = strText;

            if ( ! strOut.empty() )
                strOut[ 0 ] = (char)std::toupper( (unsigned char)strOut[ 0 ] );
            return strOut;
        }

        std::string TypeOf( const std::string &strJson, std::vector<std::string> *pastrMaterials )
        {
            std::unique_ptr<IASLib::CJsonNode> pNode( RayJson::Parse( strJson ) );
            std::string                        strType;

            if ( ! pNode || ! pNode->isObject() )
                return strType;
            if ( pNode->get( "type" ) && pNode->get( "type" )->isTextual() )
                strType = pNode->get( "type" )->textString();

            const IASLib::CJsonNode *pMaterials = pNode->get( "materials" );

            if ( pastrMaterials && pMaterials && pMaterials->isArray() )
            {
                for ( int i = 0; i < pMaterials->size(); i++ )
                {
                    const IASLib::CJsonNode *pType = pMaterials->get( i ) ? pMaterials->get( i )->get( "type" ) : NULL;

                    pastrMaterials->push_back( ( pType && pType->isTextual() ) ? pType->textString() : std::string( "material" ) );
                }
            }
            return strType;
        }

        bool IsArchiveFile( const std::string &strPath )
        {
            FILE   *pFile = fopen( strPath.c_str(), "rb" );
            int     nFirst = EOF;

            if ( ! pFile )
                return false;
                // A JSON scene starts with "{" (after any white space).
            do
                nFirst = fgetc( pFile );
            while ( nFirst != EOF && std::isspace( nFirst ) );
            fclose( pFile );
            return ( nFirst != EOF && nFirst != '{' );
        }

        bool EndsWith( const std::string &strText, const std::string &strEnd )
        {
            if ( strText.size() < strEnd.size() )
                return false;
            for ( size_t i = 0; i < strEnd.size(); i++ )
            {
                if ( std::tolower( (unsigned char)strText[ strText.size() - strEnd.size() + i ] ) != std::tolower( (unsigned char)strEnd[ i ] ) )
                    return false;
            }
            return true;
        }
    }

    CEditorDocument::CEditorDocument( void )
        : m_pScene( new CScene ), m_bArchive( false ), m_bDirty( false ), m_nRevision( 1 ), m_nUndoBytes( 0 )
    {
        New();
    }

    std::shared_ptr<const std::string> CEditorDocument::Snapshot( void )
    {
        return std::make_shared<const std::string>( m_pScene->ToJson( TRUE ) );
    }

    bool CEditorDocument::Restore( const std::string &strScene )
    {
        std::unique_ptr<CScene>     pScene( new CScene );
        RayJson::CBaseDirectory     oBase( Directory() );

        if ( pScene->FromJson( strScene ) != NO_SER_ERROR )
            return false;
        m_pScene = std::move( pScene );
        return true;
    }

    void CEditorDocument::PushUndo( const std::shared_ptr<const std::string> &pBefore )
    {
        m_apUndo.push_back( pBefore );
        m_nUndoBytes += pBefore->size();
        while ( m_apUndo.size() > 2 && ( m_apUndo.size() > UNDO_STEPS || m_nUndoBytes > UNDO_BYTES ) )
        {
            m_nUndoBytes -= m_apUndo.front()->size();
            m_apUndo.pop_front();
        }
        m_apRedo.clear();
    }

    void CEditorDocument::Changed( void )
    {
        m_bDirty = true;
        m_nRevision++;
    }

    void CEditorDocument::New( void )
    {
        std::unique_ptr<CScene> pScene( new CScene );

        pScene->FromJson( NEW_SCENE );
        m_pScene = std::move( pScene );
        m_strPath.clear();
        m_bArchive = false;
        m_bDirty = false;
        m_apUndo.clear();
        m_apRedo.clear();
        m_nUndoBytes = 0;
        m_nRevision++;
    }

    bool CEditorDocument::Load( const std::string &strPath, std::string &strError )
    {
        std::unique_ptr<CScene> pScene( new CScene );
        DWORD                   nResult = pScene->Load( strPath.c_str() );

        if ( nResult != NO_SER_ERROR )
        {
            strError = pScene->GetLoadError();
            if ( strError.empty() )
                strError = ( nResult == WRONG_VERSION ) ? "the scene file is a version this editor can't read" : "the file can't be read";
            return false;
        }
        m_pScene = std::move( pScene );
        m_strPath = strPath;
        m_bArchive = IsArchiveFile( strPath );
        m_bDirty = false;
        m_apUndo.clear();
        m_apRedo.clear();
        m_nUndoBytes = 0;
        m_nRevision++;
        return true;
    }

    bool CEditorDocument::Save( const std::string &strPath, std::string &strError )
    {
        bool    bJson = EndsWith( strPath, ".json" );
        DWORD   nResult = bJson ? m_pScene->SaveJson( strPath.c_str() ) : m_pScene->Save( strPath.c_str() );

        if ( nResult != NO_SER_ERROR )
        {
            strError = m_pScene->GetLoadError();
            if ( strError.empty() )
                strError = "the file can't be written";
            return false;
        }
        m_strPath = strPath;
        m_bArchive = ! bJson;
        m_bDirty = false;
        return true;
    }

    bool CEditorDocument::ExportObject( const std::string &strPart, const std::string &strPath, std::string &strError )
    {
        const std::string strPrefix = "objects/";
        int               nIndex = -1;

        if ( strPart.compare( 0, strPrefix.size(), strPrefix ) == 0 && strPart.size() > strPrefix.size() &&
             strPart.find_first_not_of( "0123456789", strPrefix.size() ) == std::string::npos )
            nIndex = atoi( strPart.c_str() + strPrefix.size() );
        if ( nIndex < 0 || nIndex >= m_pScene->GetObjectCount() )
        {
            strError = "choose an object to export";
            return false;
        }
        if ( m_pScene->SaveObject( nIndex, strPath.c_str(), EndsWith( strPath, ".json" ) ? TRUE : FALSE ) != NO_SER_ERROR )
        {
            strError = m_pScene->GetLoadError();
            if ( strError.empty() )
                strError = "the file can't be written";
            return false;
        }
        return true;
    }

    std::string CEditorDocument::Directory( void ) const
    {
        size_t nSlash = m_strPath.find_last_of( "/\\" );

        return ( nSlash == std::string::npos ) ? std::string() : m_strPath.substr( 0, nSlash );
    }

    std::vector<SPart> CEditorDocument::Parts( void )
    {
        std::vector<SPart> aoParts;

        aoParts.push_back( { "settings", "Settings", "", {} } );
        aoParts.push_back( { "camera", "Camera", "", {} } );
        if ( m_pScene->GetEnvironment() )
            aoParts.push_back( { "environment", "Environment", "", {} } );
        if ( m_pScene->GetHaze() )
            aoParts.push_back( { "haze", "Haze", "", {} } );
        for ( int i = 0; i < m_pScene->GetObjectCount(); i++ )
        {
            SPart oPart;

            oPart.strPart = "objects/" + std::to_string( i );
            oPart.strType = TypeOf( m_pScene->PartToJson( oPart.strPart ), &oPart.astrMaterials );
            oPart.strLabel = std::to_string( i + 1 ) + ". " + Capitalized( oPart.strType.empty() ? std::string( "object" ) : oPart.strType );
            aoParts.push_back( oPart );
        }
        for ( int i = 0; i < m_pScene->GetLightCount(); i++ )
        {
            SPart oPart;

            oPart.strPart = "lights/" + std::to_string( i );
            oPart.strType = TypeOf( m_pScene->PartToJson( oPart.strPart ), NULL );
            oPart.strLabel = std::to_string( i + 1 ) + ". " + Capitalized( oPart.strType.empty() ? std::string( "light" ) : oPart.strType );
            aoParts.push_back( oPart );
        }
        return aoParts;
    }

    std::string CEditorDocument::PartJson( const std::string &strPart )
    {
        return m_pScene->PartToJson( strPart );
    }

    bool CEditorDocument::SetPart( const std::string &strPart, const std::string &strText, std::string &strError )
    {
        std::shared_ptr<const std::string>  pBefore = Snapshot();
        DWORD                               nResult = m_pScene->EditPart( strPart, strText, Directory(), m_bArchive ? m_strPath : std::string() );

        if ( nResult != NO_SER_ERROR )
        {
            strError = m_pScene->GetLoadError();
            if ( strError.empty() )
                strError = "the scene doesn't accept that";
            return false;
        }
        PushUndo( pBefore );
        Changed();
        return true;
    }

    bool CEditorDocument::Add( const std::string &strList, const std::string &strText, std::string &strPart, std::string &strError )
    {
        if ( strList != "objects" && strList != "lights" )
        {
            strError = "only objects and lights can be added";
            return false;
        }
            // The new part is the first added (an include adds what it
            // brings in after itself).
        int nCount = ( strList == "objects" ) ? m_pScene->GetObjectCount() : m_pScene->GetLightCount();

        if ( ! SetPart( strList + "/+", strText, strError ) )
            return false;
        strPart = strList + "/" + std::to_string( nCount );
        return true;
    }

    bool CEditorDocument::Delete( const std::string &strPart, std::string &strError )
    {
        if ( strPart.compare( 0, 8, "objects/" ) != 0 && strPart.compare( 0, 7, "lights/" ) != 0 && strPart != "environment" &&
             strPart != "haze" )
        {
            strError = "the settings and the camera can't be deleted";
            return false;
        }
        return SetPart( strPart, std::string(), strError );
    }

    bool CEditorDocument::Duplicate( const std::string &strPart, std::string &strNew, std::string &strError )
    {
        std::string strText = PartJson( strPart );
        size_t      nSlash = strPart.find( '/' );

        if ( strText.empty() || nSlash == std::string::npos )
        {
            strError = "only objects and lights can be duplicated";
            return false;
        }
        return Add( strPart.substr( 0, nSlash ), strText, strNew, strError );
    }

    bool CEditorDocument::ImportModel( const std::string &strFile, std::string &strPart, std::string &strError )
    {
        std::string strEscaped;

        for ( char ch : strFile )
        {
            if ( ch == '\\' || ch == '"' )
                strEscaped += '\\';
            strEscaped += ch;
        }

        std::string strText = "{ \"type\": \"mesh\", \"file\": \"" + strEscaped + "\", \"fitSize\": 2.0, \"align\": \"bottom\", "
                              "\"smooth\": true, \"doubleSided\": true, " + MATERIALS;

        return Add( "objects", strText, strPart, strError );
    }

    bool CEditorDocument::Undo( void )
    {
        if ( m_apUndo.empty() )
            return false;

        std::shared_ptr<const std::string> pNow = Snapshot();
        std::shared_ptr<const std::string> pBack = m_apUndo.back();

        if ( ! Restore( *pBack ) )
            return false;
        m_nUndoBytes -= pBack->size();
        m_apUndo.pop_back();
        m_apRedo.push_back( pNow );
        Changed();
        return true;
    }

    bool CEditorDocument::Redo( void )
    {
        if ( m_apRedo.empty() )
            return false;

        std::shared_ptr<const std::string> pNow = Snapshot();
        std::shared_ptr<const std::string> pForward = m_apRedo.back();

        if ( ! Restore( *pForward ) )
            return false;
        m_apRedo.pop_back();
        m_apUndo.push_back( pNow );
        m_nUndoBytes += pNow->size();
        Changed();
        return true;
    }

    std::vector<std::string> CEditorDocument::ObjectTypes( void )
    {
        return { "sphere", "box", "plane", "cylinder", "cone", "triangle", "torus", "disc", "annulus", "paraboloid", "hyperboloid",
                 "superquadric", "roundedBox", "implicit", "blob", "fractal" };
    }

    std::vector<std::string> CEditorDocument::LightTypes( void )
    {
        return { "point", "spot", "directional", "sphereLight", "rectangleLight" };
    }

    std::string CEditorDocument::NewPartJson( const std::string &strType, std::string &strList )
    {
        strList = "objects";
        if ( strType == "sphere" )
            return std::string( "{ \"type\": \"sphere\", \"center\": [0.0, 1.0, 0.0], \"radius\": 1.0, " ) + MATERIALS;
        if ( strType == "box" )
            return std::string( "{ \"type\": \"box\", \"center\": [0.0, 0.5, 0.0], \"size\": 1.0, \"rotation\": [0.0, 0.0, 0.0], " ) + MATERIALS;
        if ( strType == "plane" )
            return std::string( "{ \"type\": \"plane\", \"position\": [0.0, 0.0, 0.0], \"normal\": [0.0, 1.0, 0.0], " ) + MATERIALS;
        if ( strType == "cylinder" )
            return std::string( "{ \"type\": \"cylinder\", \"base\": [0.0, 0.0, 0.0], \"top\": [0.0, 2.0, 0.0], \"radius\": 0.5, \"capped\": true, " ) + MATERIALS;
        if ( strType == "cone" )
            return std::string( "{ \"type\": \"cone\", \"base\": [0.0, 0.0, 0.0], \"top\": [0.0, 2.0, 0.0], \"baseRadius\": 0.7, \"topRadius\": 0.0, \"capped\": true, " ) + MATERIALS;
        if ( strType == "triangle" )
            return std::string( "{ \"type\": \"triangle\", \"corners\": [[-1.0, 0.0, 0.0], [0.0, 1.5, 0.0], [1.0, 0.0, 0.0]], \"doubleSided\": true, " ) + MATERIALS;
        if ( strType == "torus" )
            return std::string( "{ \"type\": \"torus\", \"center\": [0.0, 0.3, 0.0], \"majorRadius\": 1.0, \"minorRadius\": 0.3, " ) + MATERIALS;
        if ( strType == "disc" )
            return std::string( "{ \"type\": \"disc\", \"center\": [0.0, 0.01, 0.0], \"radius\": 1.0, " ) + MATERIALS;
        if ( strType == "annulus" )
            return std::string( "{ \"type\": \"annulus\", \"center\": [0.0, 0.01, 0.0], \"radius\": 1.0, \"innerRadius\": 0.5, " ) + MATERIALS;
        if ( strType == "paraboloid" )
            return std::string( "{ \"type\": \"paraboloid\", \"center\": [0.0, 0.0, 0.0], \"radius\": 1.0, \"height\": 1.0, \"capped\": false, " ) + MATERIALS;
        if ( strType == "hyperboloid" )
            return std::string( "{ \"type\": \"hyperboloid\", \"center\": [0.0, 1.0, 0.0], \"radius\": 0.5, \"topRadius\": 0.8, \"height\": 2.0, \"capped\": true, " ) + MATERIALS;
        if ( strType == "superquadric" )
            return std::string( "{ \"type\": \"superquadric\", \"center\": [0.0, 1.0, 0.0], \"radius\": 1.0, \"e1\": 0.5, \"e2\": 0.5, " ) + MATERIALS;
        if ( strType == "roundedBox" )
            return std::string( "{ \"type\": \"roundedBox\", \"center\": [0.0, 0.5, 0.0], \"size\": 1.0, \"radius\": 0.15, " ) + MATERIALS;

        if ( strType == "implicit" )
            return std::string( "{ \"type\": \"implicit\", \"center\": [0.0, 1.0, 0.0], \"formula\": \"x^2 + y^2 + z^2 - 1\", \"bounds\": 1.0, " ) + MATERIALS;
        if ( strType == "blob" )
            return std::string( "{ \"type\": \"blob\", \"center\": [0.0, 0.0, 0.0], \"balls\": [ { \"center\": [-0.5, 1.0, 0.0], \"radius\": 1.2 }, "
                                "{ \"center\": [0.5, 1.0, 0.0], \"radius\": 1.2 } ], " ) + MATERIALS;
        if ( strType == "fractal" )
            return std::string( "{ \"type\": \"fractal\", \"center\": [0.0, 1.0, 0.0], \"kind\": \"menger\", " ) + MATERIALS;

        strList = "lights";
        if ( strType == "point" )
            return "{ \"type\": \"point\", \"position\": [0.0, 6.0, -4.0], \"color\": [0.8, 0.8, 0.8] }";
        if ( strType == "spot" )
            return "{ \"type\": \"spot\", \"position\": [0.0, 6.0, -4.0], \"direction\": [0.0, -1.0, 0.6], \"color\": [0.8, 0.8, 0.8], "
                   "\"angle\": 25.0, \"falloff\": 5.0 }";
        if ( strType == "directional" )
            return "{ \"type\": \"directional\", \"direction\": [0.3, -1.0, 0.4], \"color\": [0.6, 0.6, 0.6] }";
        if ( strType == "sphereLight" )
            return "{ \"type\": \"sphereLight\", \"position\": [0.0, 6.0, -4.0], \"color\": [0.8, 0.8, 0.8], \"radius\": 0.5, \"samples\": 4 }";
        if ( strType == "rectangleLight" )
            return "{ \"type\": \"rectangleLight\", \"position\": [0.0, 6.0, 0.0], \"color\": [0.8, 0.8, 0.8], "
                   "\"edge1\": [2.0, 0.0, 0.0], \"edge2\": [0.0, 0.0, 2.0], \"samples\": 4 }";

        strList.clear();
        return std::string();
    }
}
