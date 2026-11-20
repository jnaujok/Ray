/*************************************************************************
*   RayJson                                                              *
*                                                                        *
*   Helpers for reading and writing scenes as JSON with IASLib's JSON    *
*   trees: typed field access with clear error messages, vectors and     *
*   colors as [x, y, z] arrays, angles in degrees, and a file writer     *
*   that keeps short arrays on one line and rounds numbers to six        *
*   decimal places.                                                      *
*                                                                        *
*   Reading functions return FALSE on a bad value and record why; use    *
*   CWhere to say where in the file the reader is, so the message can    *
*   name the place ("objects[2].materials[0]: radius must be a number"). *
*   A field that is missing is an error only when it is required;        *
*   otherwise the value is left as it was, so it keeps its default.      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __SCENEJSON_H__
#define __SCENEJSON_H__

#include "Globals.h"

#include "JSON/JsonNode.h"
#include "JSON/ObjectNode.h"
#include "JSON/ArrayNode.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CVector;

namespace IASLib
{
    class CResourceFile;
}
class CColor;

namespace RayJson
{
        /// Names a place in the file while it is read, such as "camera" or
        /// "objects[2]". They nest, and unwind when they go out of scope.
    class CWhere
    {
        public:
            explicit    CWhere( const char *strName );
                        CWhere( const char *strName, int nIndex );
                       ~CWhere( void );
        private:
                        CWhere( const CWhere & );
            CWhere     &operator =( const CWhere & );
    };

        /// While one exists (on this thread), scenes written as JSON carry
        /// the data of the files they use, such as texture images, so a copy
        /// sent to a task needs no files. Scene files leave them out.
    class CEmbedData
    {
        public:
                        CEmbedData( void );
                       ~CEmbedData( void );
        private:
                        CEmbedData( const CEmbedData & );
            CEmbedData &operator =( const CEmbedData & );
    };
    BOOL            EmbeddingData( void );

        /// While one exists, relative file names in a scene being read are
        /// taken from this directory (the scene file's).
    class CBaseDirectory
    {
        public:
            explicit    CBaseDirectory( const std::string &strDirectory );
                       ~CBaseDirectory( void );
        private:
            std::string m_strPrevious;
                        CBaseDirectory( const CBaseDirectory & );
            CBaseDirectory &operator =( const CBaseDirectory & );
    };
        /// A file name from a scene, relative to the scene file's
        /// directory, as an absolute path.
    std::string     ResolvePath( const std::string &strPath );

        /// While one exists, file names written into a scene are made
        /// relative to this directory (the one the scene is being saved to).
    class CSaveDirectory
    {
        public:
            explicit    CSaveDirectory( const std::string &strDirectory );
                       ~CSaveDirectory( void );
        private:
            std::string m_strPrevious;
                        CSaveDirectory( const CSaveDirectory & );
            CSaveDirectory &operator =( const CSaveDirectory & );
    };
        /// How to write an absolute path into the scene being saved: relative
        /// to its directory, or strFallback when no scene is being saved.
    std::string     PathForSave( const std::string &strAbsolute, const std::string &strFallback );

        /// While one exists, files a scene names are looked for in this
        /// archive (by name) before the disk.
    class CArchiveSource
    {
        public:
            explicit    CArchiveSource( IASLib::CResourceFile &oArchive );
                       ~CArchiveSource( void );
        private:
            IASLib::CResourceFile *m_pPrevious;
                        CArchiveSource( const CArchiveSource & );
            CArchiveSource &operator =( const CArchiveSource & );
    };

        /// Reads a file a scene names: from the archive being read, if it has
        /// one by that name, else from the disk (relative to the scene file).
        /// strPath is where on the disk it came from (empty: the archive).
    BOOL            ReadSceneFile( const std::string &strName, std::string &strData, std::string &strPath, std::string &strError );

        /// While one exists, files a scene names are collected as it is
        /// written, to go into an archive (or out beside a JSON file).
    class CFileCollector
    {
        public:
                        CFileCollector( void );
                       ~CFileCollector( void );
                /// The files, name and data, in the order they were named.
            const std::vector< std::pair< std::string, std::shared_ptr<const std::string> > > &Files( void ) const { return m_aoFiles; }
                /// Adds a file; returns the name to write in the scene, which
                /// is strName unless another file already has it.
            std::string Add( const std::string &strName, const std::shared_ptr<const std::string> &pData );
        private:
            std::vector< std::pair< std::string, std::shared_ptr<const std::string> > > m_aoFiles;
            CFileCollector *m_pPrevious;
                        CFileCollector( const CFileCollector & );
            CFileCollector &operator =( const CFileCollector & );
    };

        /// The name to write in a scene for a file it uses: with a collector,
        /// the file goes in with it (pData; strName is its name); otherwise a
        /// path relative to where the scene is saved (from strAbsolute).
    std::string     SceneFileName( const std::string &strName, const std::string &strAbsolute,
                                   const std::shared_ptr<const std::string> &pData );

        /// Records an error at the current place and returns FALSE.
    BOOL            Fail( const std::string &strMessage );
        /// The last error recorded, with its place.
    const std::string &GetError( void );
    void            ClearError( void );

        /// The JSON object field strKey, or NULL if it is missing. Fails (and
        /// returns NULL) if oNode is not an object.
    const IASLib::CJsonNode *Field( const IASLib::CJsonNode &oNode, const char *strKey, BOOL bRequired );

        /// Fails if oNode has a field not in astrFields (a NULL-terminated
        /// list; "type" is always allowed), so typos are reported.
    BOOL            OnlyFields( const IASLib::CJsonNode &oNode, const char *const *astrFields );
        /// The "type" of a JSON object, or of a plain string ("corners" for
        /// { "type": "corners" }). Fails if there is none.
    BOOL            GetType( const IASLib::CJsonNode &oNode, std::string &strType );

    BOOL            GetNumber( const IASLib::CJsonNode &oNode, const char *strKey, FLT &nValue, BOOL bRequired );
    BOOL            GetInt( const IASLib::CJsonNode &oNode, const char *strKey, int &nValue, int nMin, int nMax, BOOL bRequired );
    BOOL            GetBool( const IASLib::CJsonNode &oNode, const char *strKey, BOOL &bValue, BOOL bRequired );
    BOOL            GetString( const IASLib::CJsonNode &oNode, const char *strKey, std::string &strValue, BOOL bRequired );
    BOOL            GetVector( const IASLib::CJsonNode &oNode, const char *strKey, CVector &oValue, BOOL bRequired );
    BOOL            GetColor( const IASLib::CJsonNode &oNode, const char *strKey, CColor &oValue, BOOL bRequired );
        /// An angle, given in degrees.
    BOOL            GetAngle( const IASLib::CJsonNode &oNode, const char *strKey, FLT &nRadians, BOOL bRequired );
        /// A vector given as a JSON [x, y, z] array.
    BOOL            ToVector( const IASLib::CJsonNode &oNode, CVector &oValue );

        /// A number rounded as scene files write it: to six decimal places.
    double          Rounded( double dValue );

    void            PutNumber( IASLib::CObjectNode &oNode, const char *strKey, FLT nValue );
    void            PutBool( IASLib::CObjectNode &oNode, const char *strKey, BOOL bValue );
    void            PutVector( IASLib::CObjectNode &oNode, const char *strKey, const CVector &oValue );
    void            PutColor( IASLib::CObjectNode &oNode, const char *strKey, const CColor &oValue );
        /// Adds an [x, y, z] array to an array.
    void            AddVector( IASLib::CArrayNode &oArray, const CVector &oValue );
        /// Writes an angle in degrees.
    void            PutAngle( IASLib::CObjectNode &oNode, const char *strKey, FLT nRadians );

        /// JSON text for a scene file: indented by four spaces, arrays of
        /// numbers kept on one line, and floating-point numbers rounded to
        /// six decimal places. (Compact text for the network is not rounded.)
    std::string     Write( const IASLib::CJsonNode &oNode );
        /// Parses JSON text. Returns NULL (and records why) if it is not
        /// valid JSON. The caller owns the tree.
    IASLib::CJsonNode *Parse( const std::string &strText );
}

#endif // __SCENEJSON_H__
