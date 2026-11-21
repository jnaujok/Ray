/*************************************************************************
*   RayPresets                                                           *
*                                                                        *
*   A library of natural materials that a scene can pull in by name and  *
*   then change: marbles, granites and other stones, woods, gems,        *
*   metals and the like, each a solid texture and a finish.              *
*                                                                        *
*   A preset can be named in two places:                                 *
*                                                                        *
*     - as a texture, anywhere a texture goes (a material's, a bump      *
*       map's, a light's filter): { "preset": "carrara-marble" } is the  *
*       preset's texture, and any other field given beside it ("scale",  *
*       "colors", "seed", ...) replaces the preset's;                    *
*                                                                        *
*     - as a whole finish, in an object's "materials" list:              *
*       { "preset": "carrara-marble" } stands for the preset's materials *
*       (ambient, diffuse with the texture, a highlight, a little        *
*       reflection, as suits it). Beside it, "texture" is laid over the  *
*       texture of every material that has one, and a material type      *
*       ("ambient", "diffuse", "specular", "reflective", "transparent")  *
*       is laid over the preset's material of that type, added if the    *
*       preset has none, or taken out if it is null.                     *
*                                                                        *
*   Laying over means objects merge field by field and anything else is  *
*   replaced. A scene saved again keeps the preset's name and only what  *
*   was given beside it, as long as the result is still exactly that;    *
*   once it has been changed some other way (in an editor), the full     *
*   texture or materials are saved instead.                              *
*                                                                        *
*   The presets are kept in this library, not in files, so a scene that  *
*   names one needs nothing else and networked engines have them too.    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __PRESETS_H__
#define __PRESETS_H__

#include "Globals.h"

#include <string>

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
    class CArrayNode;
}

namespace RayPresets
{
    struct SPreset
    {
        const char     *strName;
        const char     *strGroup;           // "stone", "wood", "gem", "metal", "other"
        const char     *strDescription;
        const char     *strTexture;         // the texture, as JSON
        const char     *strMaterials;       // the finish, as a JSON array of materials; NULL for a texture only
    };

        /// Every preset, and how many there are.
    const SPreset          *GetPresets( int &nCount );
        /// The preset of that name, or NULL.
    const SPreset          *Find( const std::string &strName );

        /// The texture a texture object with a "preset" stands for: the
        /// preset's texture with the object's other fields laid over it. A
        /// new object the caller owns; NULL, with RayJson::Fail, if there is
        /// no such preset.
    IASLib::CObjectNode    *ExpandTexture( const IASLib::CJsonNode &oGiven );
        /// The materials a materials-list entry with a "preset" stands for
        /// (see above). A new array the caller owns; NULL, with
        /// RayJson::Fail, on an unknown preset or field.
    IASLib::CArrayNode     *ExpandMaterials( const IASLib::CJsonNode &oGiven );

        /// Lays oOver over oBase: objects merge field by field, anything
        /// else replaces what was there.
    void                    Overlay( IASLib::CObjectNode &oBase, const IASLib::CJsonNode &oOver );
}

#endif // __PRESETS_H__
