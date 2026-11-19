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

#include <string>

class CVector;
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
