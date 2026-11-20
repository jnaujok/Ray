#ifdef RAY_GUI
#include "StdAfx.h"
#else
#include "Globals.h"
#endif

#include "SceneJson.h"
#include "Vector.h"
#include "Color.h"

#include "JSON/JsonParser.h"
#include "Resources/ResourceFile.h"

#include <math.h>
#include <stdio.h>

#include <filesystem>
#include <vector>

namespace
{
        // Errors are per thread: engines decode scenes on their own threads.
    thread_local std::vector<std::string>   t_astrWhere;
    thread_local std::string                t_strError;
    thread_local int                        t_nEmbedData = 0;
    thread_local std::string                t_strBaseDirectory;
    thread_local std::string                t_strSaveDirectory;
    thread_local IASLib::CResourceFile     *t_pArchive = NULL;
    thread_local RayJson::CFileCollector   *t_pCollector = NULL;

    std::string Where( void )
    {
        std::string strWhere;

        for ( size_t nIndex = 0; nIndex < t_astrWhere.size(); nIndex++ )
        {
            const std::string &strPart = t_astrWhere[ nIndex ];

            if ( ! strWhere.empty() && strPart[ 0 ] != '[' )
                strWhere += '.';
            strWhere += strPart;
        }
        return strWhere;
    }

    const char *TypeName( const IASLib::CJsonNode &oNode )
    {
        switch ( oNode.getNodeType() )
        {
            case IASLib::JSON_OBJECT:   return "an object";
            case IASLib::JSON_ARRAY:    return "an array";
            case IASLib::JSON_STRING:   return "a string";
            case IASLib::JSON_NUMBER:   return "a number";
            case IASLib::JSON_BOOLEAN:  return "true or false";
            case IASLib::JSON_NULL:     return "null";
            default:                    return "something else";
        }
    }

    BOOL WrongType( const char *strKey, const char *strWanted, const IASLib::CJsonNode &oNode )
    {
        return RayJson::Fail( std::string( strKey ) + " must be " + strWanted + ", not " + TypeName( oNode ) );
    }

    BOOL Finite( const char *strKey, double dValue )
    {
        if ( ! isfinite( dValue ) )
            return RayJson::Fail( std::string( strKey ) + " is not a finite number" );
        return TRUE;
    }

        // A value as JSON text for a scene file. Floating-point numbers are
        // rounded to six decimal places, which is far finer than any scene
        // needs and keeps files readable (0.7, not 0.6999999997206032).
    std::string ScalarText( const IASLib::CJsonNode &oNode )
    {
        if ( ! oNode.isNumber() || oNode.isIntegralNumber() )
            return oNode.toJson( -1 );

        double dValue = RayJson::Rounded( oNode.doubleValue() );

        if ( ! isfinite( dValue ) || fabs( dValue ) >= 1e15 )
            return oNode.toJson( -1 );
        if ( dValue == 0.0 )
            dValue = 0.0;       // no "-0.0"

        char achBuffer[ 64 ];
        snprintf( achBuffer, sizeof( achBuffer ), "%.6f", dValue );

        std::string strText = achBuffer;
        size_t      nLast = strText.find_last_not_of( '0' );

            // Keep one decimal place, so the number still reads as a real one.
        if ( strText[ nLast ] == '.' )
            nLast++;
        strText.erase( nLast + 1 );
        return strText;
    }

    void EscapeKey( std::string &strOut, const std::string &strKey )
    {
        strOut += '"';
        for ( size_t nIndex = 0; nIndex < strKey.size(); nIndex++ )
        {
            unsigned char c = (unsigned char)strKey[ nIndex ];

            if ( c == '"' || c == '\\' )
            {
                strOut += '\\';
                strOut += (char)c;
            }
            else if ( c < 0x20 )
            {
                char achBuffer[ 8 ];
                snprintf( achBuffer, sizeof( achBuffer ), "\\u%04x", c );
                strOut += achBuffer;
            }
            else
                strOut += (char)c;
        }
        strOut += '"';
    }

    bool IsScalarArray( const IASLib::CJsonNode &oNode )
    {
        if ( ! oNode.isArray() )
            return false;
        for ( int nIndex = 0; nIndex < oNode.size(); nIndex++ )
        {
            const IASLib::CJsonNode *pChild = oNode.get( nIndex );
            if ( pChild == NULL || pChild->isContainerNode() )
                return false;
        }
        return true;
    }

    void WriteNode( std::string &strOut, const IASLib::CJsonNode &oNode, int nLevel )
    {
        const std::string strIndent( (size_t)( nLevel + 1 ) * 4, ' ' );
        const std::string strClose( (size_t)nLevel * 4, ' ' );

        if ( oNode.isObject() )
        {
            const IASLib::CObjectNode &oObject = static_cast<const IASLib::CObjectNode &>( oNode );

            if ( oObject.size() == 0 )
            {
                strOut += "{}";
                return;
            }
            strOut += "{\n";
            for ( int nIndex = 0; nIndex < oObject.size(); nIndex++ )
            {
                const std::string &strKey = oObject.fieldNameString( nIndex );

                strOut += strIndent;
                EscapeKey( strOut, strKey );
                strOut += ": ";
                WriteNode( strOut, *oObject.get( strKey ), nLevel + 1 );
                if ( nIndex + 1 < oObject.size() )
                    strOut += ',';
                strOut += '\n';
            }
            strOut += strClose + "}";
        }
        else if ( oNode.isArray() )
        {
            if ( oNode.size() == 0 )
                strOut += "[]";
            else if ( IsScalarArray( oNode ) )
            {
                    // Short and flat, like a vector: keep it on one line.
                strOut += '[';
                for ( int nIndex = 0; nIndex < oNode.size(); nIndex++ )
                {
                    if ( nIndex )
                        strOut += ", ";
                    strOut += ScalarText( *oNode.get( nIndex ) );
                }
                strOut += ']';
            }
            else
            {
                strOut += "[\n";
                for ( int nIndex = 0; nIndex < oNode.size(); nIndex++ )
                {
                    strOut += strIndent;
                    WriteNode( strOut, *oNode.get( nIndex ), nLevel + 1 );
                    if ( nIndex + 1 < oNode.size() )
                        strOut += ',';
                    strOut += '\n';
                }
                strOut += strClose + "]";
            }
        }
        else
            strOut += ScalarText( oNode );
    }
}

namespace RayJson
{
    CWhere::CWhere( const char *strName )
    {
        t_astrWhere.push_back( strName );
    }

    CWhere::CWhere( const char *strName, int nIndex )
    {
        t_astrWhere.push_back( std::string( strName ) + "[" + std::to_string( nIndex ) + "]" );
    }

    CWhere::~CWhere( void )
    {
        t_astrWhere.pop_back();
    }

    CEmbedData::CEmbedData( void )
    {
        t_nEmbedData++;
    }

    CEmbedData::~CEmbedData( void )
    {
        t_nEmbedData--;
    }

    BOOL EmbeddingData( void )
    {
        return ( t_nEmbedData > 0 ) ? TRUE : FALSE;
    }

    CBaseDirectory::CBaseDirectory( const std::string &strDirectory ) :
        m_strPrevious( t_strBaseDirectory )
    {
        t_strBaseDirectory = strDirectory;
    }

    CBaseDirectory::~CBaseDirectory( void )
    {
        t_strBaseDirectory = m_strPrevious;
    }

    std::string ResolvePath( const std::string &strPath )
    {
        std::filesystem::path oPath( strPath );

        if ( oPath.is_relative() && ! t_strBaseDirectory.empty() )
            oPath = std::filesystem::path( t_strBaseDirectory ) / oPath;
        return std::filesystem::absolute( oPath ).lexically_normal().string();
    }

    CSaveDirectory::CSaveDirectory( const std::string &strDirectory ) :
        m_strPrevious( t_strSaveDirectory )
    {
        t_strSaveDirectory = std::filesystem::absolute( std::filesystem::path( strDirectory.empty() ? "." : strDirectory ) )
                                 .lexically_normal().string();
    }

    CSaveDirectory::~CSaveDirectory( void )
    {
        t_strSaveDirectory = m_strPrevious;
    }

    std::string PathForSave( const std::string &strAbsolute, const std::string &strFallback )
    {
        if ( t_strSaveDirectory.empty() || strAbsolute.empty() )
            return strFallback;

        std::filesystem::path oFile( strAbsolute );
        std::filesystem::path oSaveDirectory( t_strSaveDirectory );
        std::filesystem::path oRelative = oFile.lexically_relative( oSaveDirectory );

            // Sharing no more than the root (or another drive): a relative
            // path would only climb to the root, so give the absolute one.
        auto itFile = oFile.begin();
        auto itSave = oSaveDirectory.begin();
        int  nShared = 0;

        while ( itFile != oFile.end() && itSave != oSaveDirectory.end() && *itFile == *itSave )
        {
            ++itFile;
            ++itSave;
            nShared++;
        }
        if ( oRelative.empty() || nShared <= ( oFile.has_root_directory() ? 1 : 0 ) + ( oFile.has_root_name() ? 1 : 0 ) )
            return oFile.generic_string();
            // Forward slashes, so the scene works on every platform.
        return oRelative.generic_string();
    }

    CArchiveSource::CArchiveSource( IASLib::CResourceFile &oArchive ) :
        m_pPrevious( t_pArchive )
    {
        t_pArchive = &oArchive;
    }

    CArchiveSource::~CArchiveSource( void )
    {
        t_pArchive = m_pPrevious;
    }

    BOOL ReadSceneFile( const std::string &strName, std::string &strData, std::string &strPath, std::string &strError )
    {
        short nFolder, nResource;

        if ( t_pArchive && t_pArchive->FindByName( strName.c_str(), nFolder, nResource ) )
        {
            std::shared_ptr<const IASLib::CResource> pResource = t_pArchive->Get( nFolder, nResource );

            if ( ! pResource )
            {
                strError = "the archive's copy of \"" + strName + "\" is damaged";
                return FALSE;
            }
            strData.assign( (const char *)pResource->GetData(), pResource->GetLength() );
            strPath.clear();
            return TRUE;
        }

        strPath = ResolvePath( strName );

        FILE   *pFile = fopen( strPath.c_str(), "rb" );
        char    achBuffer[ 65536 ];
        size_t  nRead;

        if ( pFile == NULL )
        {
            strError = "can't open \"" + strName + "\"";
            return FALSE;
        }
        strData.clear();
        while ( ( nRead = fread( achBuffer, 1, sizeof( achBuffer ), pFile ) ) > 0 )
            strData.append( achBuffer, nRead );
        fclose( pFile );
        return TRUE;
    }

    CFileCollector::CFileCollector( void ) :
        m_pPrevious( t_pCollector )
    {
        t_pCollector = this;
    }

    CFileCollector::~CFileCollector( void )
    {
        t_pCollector = m_pPrevious;
    }

    std::string CFileCollector::Add( const std::string &strName, const std::shared_ptr<const std::string> &pData )
    {
            // A name that is absolute or climbs out of the scene's directory
            // would put the file somewhere else when it is extracted: keep only
            // its file name, under "files/".
        std::filesystem::path   oName = std::filesystem::path( strName ).lexically_normal();
        std::string             strBase = oName.generic_string();

        if ( strBase.empty() || strBase == "." || oName.is_absolute() || oName.has_root_name() ||
             strBase.compare( 0, 2, ".." ) == 0 )
            strBase = "files/" + ( oName.filename().empty() ? std::string( "file" ) : oName.filename().generic_string() );

        std::string strTry = strBase;

            // The same data under the same name is one file; another file
            // with the name gets a number added.
        for ( int nTry = 2; ; nTry++ )
        {
            bool bTaken = false;

            for ( size_t nIndex = 0; nIndex < m_aoFiles.size(); nIndex++ )
            {
                if ( m_aoFiles[ nIndex ].first == strTry )
                {
                    if ( *m_aoFiles[ nIndex ].second == *pData )
                        return strTry;
                    bTaken = true;
                }
            }
            if ( ! bTaken )
                break;

            size_t nDot = strBase.find_last_of( '.' );
            size_t nSlash = strBase.find_last_of( '/' );

            if ( nDot == std::string::npos || ( nSlash != std::string::npos && nDot < nSlash ) )
                strTry = strBase + "-" + std::to_string( nTry );
            else
                strTry = strBase.substr( 0, nDot ) + "-" + std::to_string( nTry ) + strBase.substr( nDot );
        }
        m_aoFiles.push_back( std::make_pair( strTry, pData ) );
        return strTry;
    }

    std::string SceneFileName( const std::string &strName, const std::string &strAbsolute,
                               const std::shared_ptr<const std::string> &pData )
    {
        if ( t_pCollector && pData )
            return t_pCollector->Add( strName, pData );
        return PathForSave( strAbsolute, strName );
    }

    BOOL Fail( const std::string &strMessage )
    {
            // Keep the first error: later ones are usually caused by it.
        if ( t_strError.empty() )
        {
            std::string strWhere = Where();
            t_strError = strWhere.empty() ? strMessage : strWhere + ": " + strMessage;
        }
        return FALSE;
    }

    const std::string &GetError( void )
    {
        return t_strError;
    }

    void ClearError( void )
    {
        t_strError.clear();
    }

    const IASLib::CJsonNode *Field( const IASLib::CJsonNode &oNode, const char *strKey, BOOL bRequired )
    {
        if ( ! oNode.isObject() )
        {
            Fail( std::string( "must be an object, not " ) + TypeName( oNode ) );
            return NULL;
        }

        const IASLib::CJsonNode *pField = oNode.get( strKey );

        if ( pField == NULL && bRequired )
            Fail( std::string( "needs \"" ) + strKey + "\"" );
        return pField;
    }

    BOOL OnlyFields( const IASLib::CJsonNode &oNode, const char *const *astrFields )
    {
        if ( ! oNode.isObject() )
            return Fail( std::string( "must be an object, not " ) + TypeName( oNode ) );

        const IASLib::CObjectNode &oObject = static_cast<const IASLib::CObjectNode &>( oNode );

        for ( int nIndex = 0; nIndex < oObject.size(); nIndex++ )
        {
            const std::string  &strKey = oObject.fieldNameString( nIndex );
            bool                bKnown = ( strKey == "type" );

            for ( int nField = 0; ! bKnown && astrFields[ nField ]; nField++ )
                bKnown = ( strKey == astrFields[ nField ] );
            if ( ! bKnown )
                return Fail( "unknown field \"" + strKey + "\"" );
        }
        return TRUE;
    }

    BOOL GetType( const IASLib::CJsonNode &oNode, std::string &strType )
    {
        if ( oNode.isTextual() )
        {
            strType = oNode.textString();
            return TRUE;
        }
        return GetString( oNode, "type", strType, TRUE );
    }

    BOOL GetNumber( const IASLib::CJsonNode &oNode, const char *strKey, FLT &nValue, BOOL bRequired )
    {
        const IASLib::CJsonNode *pField = Field( oNode, strKey, bRequired );

        if ( pField == NULL )
            return ( oNode.isObject() && ! bRequired ) ? TRUE : FALSE;
        if ( ! pField->isNumber() )
            return WrongType( strKey, "a number", *pField );
        if ( ! Finite( strKey, pField->doubleValue() ) )
            return FALSE;
        nValue = (FLT)pField->doubleValue();
        return TRUE;
    }

    BOOL GetInt( const IASLib::CJsonNode &oNode, const char *strKey, int &nValue, int nMin, int nMax, BOOL bRequired )
    {
        const IASLib::CJsonNode *pField = Field( oNode, strKey, bRequired );

        if ( pField == NULL )
            return ( oNode.isObject() && ! bRequired ) ? TRUE : FALSE;
        if ( ! pField->isNumber() )
            return WrongType( strKey, "a number", *pField );

        double dValue = pField->doubleValue();

        if ( dValue != floor( dValue ) || dValue < nMin || dValue > nMax )
            return Fail( std::string( strKey ) + " must be a whole number from " + std::to_string( nMin ) +
                         " to " + std::to_string( nMax ) );
        nValue = (int)dValue;
        return TRUE;
    }

    BOOL GetBool( const IASLib::CJsonNode &oNode, const char *strKey, BOOL &bValue, BOOL bRequired )
    {
        const IASLib::CJsonNode *pField = Field( oNode, strKey, bRequired );

        if ( pField == NULL )
            return ( oNode.isObject() && ! bRequired ) ? TRUE : FALSE;
        if ( ! pField->isBoolean() )
            return WrongType( strKey, "true or false", *pField );
        bValue = pField->booleanValue() ? TRUE : FALSE;
        return TRUE;
    }

    BOOL GetString( const IASLib::CJsonNode &oNode, const char *strKey, std::string &strValue, BOOL bRequired )
    {
        const IASLib::CJsonNode *pField = Field( oNode, strKey, bRequired );

        if ( pField == NULL )
            return ( oNode.isObject() && ! bRequired ) ? TRUE : FALSE;
        if ( ! pField->isTextual() )
            return WrongType( strKey, "a string", *pField );
        strValue = pField->textString();
        return TRUE;
    }

    BOOL ToVector( const IASLib::CJsonNode &oNode, CVector &oValue )
    {
        FLT anValue[ 3 ];

        if ( ! oNode.isArray() || oNode.size() != 3 )
            return Fail( std::string( "must be an array of three numbers, not " ) +
                         ( oNode.isArray() ? "an array of " + std::to_string( oNode.size() ) : std::string( TypeName( oNode ) ) ) );

        for ( int nIndex = 0; nIndex < 3; nIndex++ )
        {
            const IASLib::CJsonNode *pValue = oNode.get( nIndex );

            if ( pValue == NULL || ! pValue->isNumber() )
                return Fail( "must be an array of three numbers" );
            if ( ! Finite( "every element", pValue->doubleValue() ) )
                return FALSE;
            anValue[ nIndex ] = (FLT)pValue->doubleValue();
        }
        oValue.Set( anValue[ 0 ], anValue[ 1 ], anValue[ 2 ] );
        return TRUE;
    }

    BOOL GetVector( const IASLib::CJsonNode &oNode, const char *strKey, CVector &oValue, BOOL bRequired )
    {
        const IASLib::CJsonNode *pField = Field( oNode, strKey, bRequired );

        if ( pField == NULL )
            return ( oNode.isObject() && ! bRequired ) ? TRUE : FALSE;

        CWhere oWhere( strKey );
        return ToVector( *pField, oValue );
    }

    BOOL GetColor( const IASLib::CJsonNode &oNode, const char *strKey, CColor &oValue, BOOL bRequired )
    {
        CVector oVector;
        BOOL    bPresent = ( oNode.isObject() && oNode.get( strKey ) != NULL );

        if ( ! GetVector( oNode, strKey, oVector, bRequired ) )
            return FALSE;
        if ( bPresent )
            oValue.Set( oVector.GetX(), oVector.GetY(), oVector.GetZ() );
        return TRUE;
    }

    BOOL GetAngle( const IASLib::CJsonNode &oNode, const char *strKey, FLT &nRadians, BOOL bRequired )
    {
        FLT nDegrees = nRadians / RADIANS;

        if ( ! GetNumber( oNode, strKey, nDegrees, bRequired ) )
            return FALSE;
        if ( oNode.get( strKey ) != NULL )
            nRadians = nDegrees * RADIANS;
        return TRUE;
    }

    double Rounded( double dValue )
    {
        return round( dValue * 1e6 ) / 1e6;
    }

    void PutNumber( IASLib::CObjectNode &oNode, const char *strKey, FLT nValue )
    {
        oNode.put( strKey, (double)nValue );
    }

    void PutBool( IASLib::CObjectNode &oNode, const char *strKey, BOOL bValue )
    {
        oNode.put( strKey, bValue ? true : false );
    }

    void AddVector( IASLib::CArrayNode &oArray, const CVector &oValue )
    {
        CVector oCopy( oValue );

        oArray.addArray()->add( (double)oCopy.GetX() )->add( (double)oCopy.GetY() )->add( (double)oCopy.GetZ() );
    }

    void PutVector( IASLib::CObjectNode &oNode, const char *strKey, const CVector &oValue )
    {
        CVector oCopy( oValue );

        oNode.putArray( strKey )->add( (double)oCopy.GetX() )->add( (double)oCopy.GetY() )->add( (double)oCopy.GetZ() );
    }

    void PutColor( IASLib::CObjectNode &oNode, const char *strKey, const CColor &oValue )
    {
        CColor oCopy( oValue );

        oNode.putArray( strKey )->add( (double)oCopy.GetR() )->add( (double)oCopy.GetG() )->add( (double)oCopy.GetB() );
    }

    void PutAngle( IASLib::CObjectNode &oNode, const char *strKey, FLT nRadians )
    {
        oNode.put( strKey, (double)( nRadians / RADIANS ) );
    }

    std::string Write( const IASLib::CJsonNode &oNode )
    {
        std::string strOut;

        WriteNode( strOut, oNode, 0 );
        strOut += '\n';
        return strOut;
    }

    IASLib::CJsonNode *Parse( const std::string &strText )
    {
        IASLib::CJsonParser     oParser;
        IASLib::CJsonNode      *pRoot = oParser.readTree( strText );

        if ( pRoot == NULL || oParser.hasError() )
        {
            delete pRoot;
            Fail( std::string( "not valid JSON: " ) + (const char *)oParser.getError() );
            return NULL;
        }
        return pRoot;
    }
}
