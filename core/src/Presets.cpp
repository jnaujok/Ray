/*************************************************************************
*   RayPresets                                                           *
*                                                                        *
*   The library of natural materials. See Presets.h.                     *
*                                                                        *
*   The textures follow the classic procedural recipes: marble as bands  *
*   bent by turbulence and wood as rings wobbled by noise (Perlin, "An   *
*   Image Synthesizer", SIGGRAPH 1985, and his 2002 "Improving Noise"),  *
*   stones and gems as noise or turbulence through a ramp of colors, and *
*   granite and terrazzo as cellular flecks (Worley, "A Cellular Texture *
*   Basis Function", SIGGRAPH 1996), as gathered in Ebert, Musgrave,     *
*   Peachey, Perlin and Worley, "Texturing and Modeling: A Procedural    *
*   Approach". The colors are our own, matched by eye to photographs of  *
*   each stone and wood.                                                 *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#include "Globals.h"

#include "Presets.h"
#include "SceneJson.h"

#include <memory>
#include <string.h>

    // The finishes, each a list of materials around a texture preset.
    // Polished: stones and gems with a gloss and a hint of reflection.
#define POLISHED( name ) \
    R"([ { "type": "ambient", "influence": 1.0, "color": [ 0.08, 0.08, 0.08 ] },)" \
    R"(  { "type": "diffuse", "influence": 0.85, "color": [ 1.0, 1.0, 1.0 ], "texture": { "preset": ")" name R"(" } },)" \
    R"(  { "type": "specular", "influence": 0.35, "color": [ 1.0, 1.0, 1.0 ], "power": 0.15, "indexOfRefraction": 1.5 },)" \
    R"(  { "type": "reflective", "influence": 0.04, "color": [ 1.0, 1.0, 1.0 ], "roughness": 0.05 } ])"
    // Glossy: glassy stones (obsidian) and polished resin.
#define GLOSSY( name ) \
    R"([ { "type": "ambient", "influence": 1.0, "color": [ 0.04, 0.04, 0.04 ] },)" \
    R"(  { "type": "diffuse", "influence": 0.7, "color": [ 1.0, 1.0, 1.0 ], "texture": { "preset": ")" name R"(" } },)" \
    R"(  { "type": "specular", "influence": 0.5, "color": [ 1.0, 1.0, 1.0 ], "power": 0.08, "indexOfRefraction": 1.5 },)" \
    R"(  { "type": "reflective", "influence": 0.1, "color": [ 1.0, 1.0, 1.0 ], "roughness": 0.02 } ])"
    // Satin: honed stone with a broad, faint sheen.
#define SATIN( name ) \
    R"([ { "type": "ambient", "influence": 1.0, "color": [ 0.08, 0.08, 0.08 ] },)" \
    R"(  { "type": "diffuse", "influence": 0.85, "color": [ 1.0, 1.0, 1.0 ], "texture": { "preset": ")" name R"(" } },)" \
    R"(  { "type": "specular", "influence": 0.12, "color": [ 1.0, 1.0, 1.0 ], "power": 0.5, "indexOfRefraction": 1.5 } ])"
    // Matte: rough stone, sand, concrete, fired clay, rust.
#define MATTE( name ) \
    R"([ { "type": "ambient", "influence": 1.0, "color": [ 0.08, 0.08, 0.08 ] },)" \
    R"(  { "type": "diffuse", "influence": 0.9, "color": [ 1.0, 1.0, 1.0 ], "texture": { "preset": ")" name R"(" } } ])"
    // Oiled wood: a soft highlight.
#define WOODEN( name ) \
    R"([ { "type": "ambient", "influence": 1.0, "color": [ 0.08, 0.08, 0.08 ] },)" \
    R"(  { "type": "diffuse", "influence": 0.85, "color": [ 1.0, 1.0, 1.0 ], "texture": { "preset": ")" name R"(" } },)" \
    R"(  { "type": "specular", "influence": 0.2, "color": [ 1.0, 1.0, 1.0 ], "power": 0.3, "indexOfRefraction": 1.5 } ])"
    // Brushed metal: a blurred, tinted reflection over a little color.
#define METALLIC( name ) \
    R"([ { "type": "ambient", "influence": 1.0, "color": [ 0.03, 0.03, 0.03 ] },)" \
    R"(  { "type": "diffuse", "influence": 0.15, "color": [ 1.0, 1.0, 1.0 ], "texture": { "preset": ")" name R"(" } },)" \
    R"(  { "type": "reflective", "influence": 0.75, "color": [ 1.0, 1.0, 1.0 ], "filterReflection": true, "roughness": 0.25,)" \
    R"(    "texture": { "preset": ")" name R"(" } },)" \
    R"(  { "type": "specular", "influence": 0.4, "color": [ 1.0, 1.0, 1.0 ], "power": 0.2, "indexOfRefraction": 1.5 } ])"

namespace
{
    const RayPresets::SPreset s_aoPresets[] =
    {
        // Stones.
        { "carrara-marble", "stone", "white Carrara marble with soft gray veins",
          R"({ "type": "marble", "colors": [ [ 0.42, 0.43, 0.46 ], [ 0.72, 0.72, 0.74 ], [ 0.9, 0.9, 0.89 ], [ 0.94, 0.93, 0.91 ],
               [ 0.94, 0.93, 0.91 ], [ 0.94, 0.93, 0.91 ], [ 0.94, 0.93, 0.91 ] ],
               "scale": 0.6, "turbulence": 1.2, "octaves": 6, "seed": 7, "axis": [ 0.880451, 0.440225, 0.17609 ] })",
          POLISHED( "carrara-marble" ) },
        { "black-marquina", "stone", "black Nero Marquina marble with sharp white veins",
          R"({ "type": "marble", "colors": [ [ 0.92, 0.92, 0.9 ], [ 0.42, 0.42, 0.44 ], [ 0.07, 0.07, 0.08 ], [ 0.06, 0.06, 0.07 ],
               [ 0.06, 0.06, 0.07 ], [ 0.06, 0.06, 0.07 ], [ 0.06, 0.06, 0.07 ], [ 0.06, 0.06, 0.07 ] ],
               "scale": 0.7, "turbulence": 1.4, "octaves": 7, "seed": 11, "axis": [ 0.6, 0.75, 0.28 ] })",
          POLISHED( "black-marquina" ) },
        { "verde-marble", "stone", "dark green serpentine marble with pale veins",
          R"({ "type": "marble", "colors": [ [ 0.75, 0.85, 0.78 ], [ 0.27, 0.47, 0.34 ], [ 0.09, 0.27, 0.16 ], [ 0.06, 0.19, 0.11 ],
               [ 0.08, 0.23, 0.14 ], [ 0.06, 0.19, 0.11 ] ],
               "scale": 0.45, "turbulence": 1.6, "octaves": 7, "seed": 3, "axis": [ 0.287348, 0.957826, 0.0 ] })",
          POLISHED( "verde-marble" ) },
        { "rosso-marble", "stone", "red Rosso Levanto marble with cream veins",
          R"({ "type": "marble", "colors": [ [ 0.88, 0.82, 0.74 ], [ 0.55, 0.22, 0.18 ], [ 0.42, 0.12, 0.1 ], [ 0.36, 0.1, 0.09 ],
               [ 0.45, 0.15, 0.12 ] ],
               "scale": 0.5, "turbulence": 1.8, "octaves": 6, "seed": 19, "axis": [ 0.8, 0.2, 0.56 ] })",
          POLISHED( "rosso-marble" ) },
        { "travertine", "stone", "warm beige travertine in soft horizontal bands",
          R"({ "type": "marble", "colors": [ [ 0.72, 0.62, 0.48 ], [ 0.85, 0.77, 0.63 ], [ 0.92, 0.86, 0.74 ], [ 0.8, 0.7, 0.56 ] ],
               "scale": 0.12, "turbulence": 0.4, "octaves": 5, "seed": 5, "axis": [ 0.0, 1.0, 0.0 ] })",
          SATIN( "travertine" ) },
        { "granite", "stone", "gray and pink speckled granite",
          R"({ "type": "cells", "colors": [ [ 0.13, 0.12, 0.12 ], [ 0.55, 0.52, 0.5 ], [ 0.8, 0.78, 0.76 ], [ 0.62, 0.45, 0.42 ],
               [ 0.88, 0.86, 0.84 ], [ 0.42, 0.4, 0.39 ] ],
               "scale": 0.035, "turbulence": 0.3, "octaves": 3, "seed": 2, "mode": "flecks" })",
          POLISHED( "granite" ) },
        { "black-granite", "stone", "black Absolute granite with faint flecks",
          R"({ "type": "cells", "colors": [ [ 0.035, 0.035, 0.04 ], [ 0.06, 0.06, 0.07 ], [ 0.09, 0.09, 0.1 ], [ 0.05, 0.05, 0.055 ],
               [ 0.24, 0.24, 0.26 ] ],
               "scale": 0.025, "turbulence": 0.3, "octaves": 3, "seed": 9, "mode": "flecks" })",
          POLISHED( "black-granite" ) },
        { "slate", "stone", "blue-gray slate in fine layers",
          R"({ "type": "noise", "colors": [ [ 0.16, 0.18, 0.21 ], [ 0.24, 0.27, 0.3 ], [ 0.3, 0.33, 0.36 ], [ 0.22, 0.25, 0.28 ] ],
               "scale": 0.5, "turbulence": 0.0, "octaves": 6, "seed": 13, "stretch": [ 1.0, 0.15, 1.0 ] })",
          SATIN( "slate" ) },
        { "sandstone", "stone", "red-gold sandstone in wavy strata",
          R"({ "type": "marble", "colors": [ [ 0.8, 0.62, 0.42 ], [ 0.86, 0.7, 0.5 ], [ 0.76, 0.55, 0.36 ], [ 0.9, 0.76, 0.56 ] ],
               "scale": 0.1, "turbulence": 0.3, "octaves": 5, "seed": 17, "axis": [ 0.0, 1.0, 0.0 ] })",
          MATTE( "sandstone" ) },
        { "basalt", "stone", "dark volcanic basalt",
          R"({ "type": "noise", "colors": [ [ 0.07, 0.07, 0.08 ], [ 0.15, 0.15, 0.16 ], [ 0.21, 0.2, 0.2 ] ],
               "scale": 0.3, "turbulence": 0.8, "octaves": 6, "seed": 23 })",
          MATTE( "basalt" ) },
        { "sand", "stone", "pale beach sand",
          R"({ "type": "noise", "colors": [ [ 0.7, 0.6, 0.43 ], [ 0.83, 0.74, 0.57 ], [ 0.93, 0.86, 0.7 ] ],
               "scale": 0.03, "turbulence": 0.0, "octaves": 3, "seed": 29 })",
          MATTE( "sand" ) },
        { "concrete", "stone", "gray poured concrete",
          R"({ "type": "noise", "colors": [ [ 0.44, 0.44, 0.42 ], [ 0.57, 0.57, 0.55 ], [ 0.65, 0.65, 0.63 ] ],
               "scale": 0.05, "turbulence": 0.0, "octaves": 4, "seed": 31 })",
          MATTE( "concrete" ) },
        { "terrazzo", "stone", "cream terrazzo with chips of red, blue and ochre",
          R"({ "type": "cells", "colors": [ [ 0.9, 0.88, 0.82 ], [ 0.92, 0.9, 0.85 ], [ 0.72, 0.28, 0.24 ], [ 0.92, 0.9, 0.85 ],
               [ 0.2, 0.34, 0.5 ], [ 0.9, 0.88, 0.82 ], [ 0.84, 0.72, 0.34 ], [ 0.93, 0.91, 0.86 ] ],
               "scale": 0.05, "turbulence": 0.25, "octaves": 3, "seed": 37, "mode": "flecks" })",
          POLISHED( "terrazzo" ) },
        { "terracotta", "other", "unglazed terracotta",
          R"({ "type": "noise", "colors": [ [ 0.6, 0.29, 0.17 ], [ 0.69, 0.35, 0.21 ], [ 0.76, 0.42, 0.27 ] ],
               "scale": 0.1, "turbulence": 0.0, "octaves": 5, "seed": 41 })",
          MATTE( "terracotta" ) },
        // Woods (rings about +y; give "axis" to turn the grain).
        { "oak", "wood", "golden oak with bold grain",
          R"({ "type": "wood", "colors": [ [ 0.72, 0.55, 0.34 ], [ 0.68, 0.5, 0.3 ], [ 0.6, 0.42, 0.24 ], [ 0.42, 0.28, 0.15 ] ],
               "scale": 0.06, "turbulence": 0.35, "octaves": 5, "seed": 43, "axis": [ 0.0, 1.0, 0.0 ] })",
          WOODEN( "oak" ) },
        { "pine", "wood", "pale pine",
          R"({ "type": "wood", "colors": [ [ 0.85, 0.66, 0.42 ], [ 0.8, 0.6, 0.37 ], [ 0.72, 0.51, 0.3 ], [ 0.47, 0.3, 0.16 ] ],
               "scale": 0.1, "turbulence": 0.3, "octaves": 5, "seed": 47, "axis": [ 0.0, 1.0, 0.0 ] })",
          WOODEN( "pine" ) },
        { "walnut", "wood", "dark brown walnut",
          R"({ "type": "wood", "colors": [ [ 0.42, 0.28, 0.18 ], [ 0.36, 0.23, 0.14 ], [ 0.3, 0.18, 0.1 ], [ 0.18, 0.1, 0.06 ] ],
               "scale": 0.05, "turbulence": 0.4, "octaves": 5, "seed": 53, "axis": [ 0.0, 1.0, 0.0 ] })",
          WOODEN( "walnut" ) },
        { "cherry", "wood", "reddish cherry",
          R"({ "type": "wood", "colors": [ [ 0.7, 0.38, 0.22 ], [ 0.64, 0.33, 0.19 ], [ 0.56, 0.28, 0.15 ], [ 0.4, 0.18, 0.09 ] ],
               "scale": 0.07, "turbulence": 0.3, "octaves": 5, "seed": 59, "axis": [ 0.0, 1.0, 0.0 ] })",
          WOODEN( "cherry" ) },
        { "mahogany", "wood", "deep red-brown mahogany",
          R"({ "type": "wood", "colors": [ [ 0.5, 0.22, 0.13 ], [ 0.45, 0.19, 0.11 ], [ 0.38, 0.15, 0.08 ], [ 0.26, 0.09, 0.05 ] ],
               "scale": 0.05, "turbulence": 0.25, "octaves": 5, "seed": 61, "axis": [ 0.0, 1.0, 0.0 ] })",
          WOODEN( "mahogany" ) },
        { "ebony", "wood", "near-black ebony",
          R"({ "type": "wood", "colors": [ [ 0.1, 0.08, 0.07 ], [ 0.08, 0.065, 0.055 ], [ 0.06, 0.05, 0.04 ], [ 0.03, 0.025, 0.02 ] ],
               "scale": 0.03, "turbulence": 0.5, "octaves": 5, "seed": 67, "axis": [ 0.0, 1.0, 0.0 ] })",
          WOODEN( "ebony" ) },
        { "birch", "wood", "creamy birch with faint grain",
          R"({ "type": "wood", "colors": [ [ 0.92, 0.85, 0.7 ], [ 0.89, 0.81, 0.65 ], [ 0.85, 0.76, 0.6 ], [ 0.74, 0.64, 0.48 ] ],
               "scale": 0.12, "turbulence": 0.2, "octaves": 5, "seed": 71, "axis": [ 0.0, 1.0, 0.0 ] })",
          WOODEN( "birch" ) },
        // Gems and other minerals.
        { "jade", "gem", "green nephrite jade",
          R"({ "type": "noise", "colors": [ [ 0.12, 0.38, 0.22 ], [ 0.3, 0.6, 0.4 ], [ 0.45, 0.72, 0.52 ], [ 0.25, 0.52, 0.35 ] ],
               "scale": 0.4, "turbulence": 0.0, "octaves": 6, "seed": 73 })",
          POLISHED( "jade" ) },
        { "amber", "gem", "honey amber",
          R"({ "type": "noise", "colors": [ [ 0.55, 0.25, 0.03 ], [ 0.8, 0.45, 0.08 ], [ 0.95, 0.65, 0.2 ] ],
               "scale": 0.5, "turbulence": 0.0, "octaves": 5, "seed": 79 })",
          GLOSSY( "amber" ) },
        { "obsidian", "gem", "glassy black obsidian",
          R"({ "type": "noise", "colors": [ [ 0.02, 0.02, 0.025 ], [ 0.05, 0.05, 0.06 ], [ 0.03, 0.03, 0.035 ] ],
               "scale": 0.4, "turbulence": 0.0, "octaves": 5, "seed": 83 })",
          GLOSSY( "obsidian" ) },
        { "malachite", "gem", "malachite in concentric green bands",
          R"({ "type": "wood", "colors": [ [ 0.1, 0.45, 0.28 ], [ 0.16, 0.6, 0.38 ], [ 0.05, 0.3, 0.17 ], [ 0.02, 0.18, 0.1 ] ],
               "scale": 0.04, "turbulence": 0.6, "octaves": 6, "seed": 89, "axis": [ 0.301511, 0.904534, 0.301511 ] })",
          POLISHED( "malachite" ) },
        { "lapis-lazuli", "gem", "deep blue lapis lazuli with gold flecks",
          R"({ "type": "noise", "colors": [ [ 0.08, 0.12, 0.45 ], [ 0.12, 0.2, 0.6 ], [ 0.1, 0.16, 0.55 ], [ 0.11, 0.18, 0.58 ],
               [ 0.8, 0.68, 0.3 ] ],
               "scale": 0.2, "turbulence": 1.0, "octaves": 6, "seed": 97 })",
          POLISHED( "lapis-lazuli" ) },
        // Metals.
        { "brushed-steel", "metal", "brushed stainless steel (grain along x)",
          R"({ "type": "noise", "colors": [ [ 0.55, 0.56, 0.58 ], [ 0.7, 0.71, 0.73 ], [ 0.62, 0.63, 0.65 ] ],
               "scale": 0.02, "turbulence": 0.0, "octaves": 4, "seed": 101, "stretch": [ 40.0, 1.0, 1.0 ] })",
          METALLIC( "brushed-steel" ) },
        { "brushed-gold", "metal", "brushed gold (grain along x)",
          R"({ "type": "noise", "colors": [ [ 0.7, 0.52, 0.2 ], [ 0.85, 0.66, 0.3 ], [ 0.78, 0.58, 0.25 ] ],
               "scale": 0.02, "turbulence": 0.0, "octaves": 4, "seed": 103, "stretch": [ 40.0, 1.0, 1.0 ] })",
          METALLIC( "brushed-gold" ) },
        { "brushed-copper", "metal", "brushed copper (grain along x)",
          R"({ "type": "noise", "colors": [ [ 0.62, 0.32, 0.2 ], [ 0.78, 0.44, 0.3 ], [ 0.7, 0.38, 0.25 ] ],
               "scale": 0.02, "turbulence": 0.0, "octaves": 4, "seed": 107, "stretch": [ 40.0, 1.0, 1.0 ] })",
          METALLIC( "brushed-copper" ) },
        { "rust", "metal", "rusted iron",
          R"({ "type": "noise", "colors": [ [ 0.25, 0.1, 0.05 ], [ 0.48, 0.2, 0.08 ], [ 0.65, 0.32, 0.12 ], [ 0.55, 0.4, 0.3 ] ],
               "scale": 0.15, "turbulence": 1.2, "octaves": 6, "seed": 109 })",
          MATTE( "rust" ) },
        // Light.
        { "caustics", "other", "the web of light under water, for a light's filter (a texture only)",
          R"({ "type": "caustics", "colors": [ [ 0.35, 0.4, 0.42 ], [ 1.4, 1.5, 1.45 ] ],
               "scale": 0.8, "turbulence": 0.6, "octaves": 3, "seed": 113, "width": 0.12, "layers": 2 })",
          NULL },
    };

        // Parses a preset's JSON text; the library's own, so it parses.
    IASLib::CJsonNode *ParsePreset( const char *strText )
    {
        return RayJson::Parse( std::string( strText ) );
    }

    const char *const s_astrFinishFields[] = { "ambient", "diffuse", "specular", "reflective", "transparent", NULL };
}

const RayPresets::SPreset *RayPresets::GetPresets( int &nCount )
{
    nCount = (int)( sizeof( s_aoPresets ) / sizeof( s_aoPresets[ 0 ] ) );
    return s_aoPresets;
}

const RayPresets::SPreset *RayPresets::Find( const std::string &strName )
{
    int             nCount;
    const SPreset  *aoPresets = GetPresets( nCount );

    for ( int nIndex = 0; nIndex < nCount; nIndex++ )
    {
        if ( strName == aoPresets[ nIndex ].strName )
            return &aoPresets[ nIndex ];
    }
    return NULL;
}

void RayPresets::Overlay( IASLib::CObjectNode &oBase, const IASLib::CJsonNode &oOver )
{
    const IASLib::CObjectNode *pOver = dynamic_cast<const IASLib::CObjectNode *>( &oOver );

    if ( pOver == NULL )
        return;
    for ( int nIndex = 0; nIndex < pOver->size(); nIndex++ )
    {
        const std::string          &strKey = pOver->fieldNameString( nIndex );
        const IASLib::CJsonNode    *pValue = pOver->fieldValue( nIndex );
        IASLib::CJsonNode          *pHere = oBase.get( strKey );

        if ( pHere && pHere->isObject() && pValue->isObject() )
            Overlay( *dynamic_cast<IASLib::CObjectNode *>( pHere ), *pValue );
        else
            oBase.set( strKey, pValue->clone() );
    }
}

namespace
{
        // The named preset, or NULL with an error.
    const RayPresets::SPreset *Named( const IASLib::CJsonNode &oGiven )
    {
        std::string strName;

        if ( ! RayJson::GetString( oGiven, "preset", strName, TRUE ) )
            return NULL;

        const RayPresets::SPreset *pPreset = RayPresets::Find( strName );

        if ( pPreset == NULL )
        {
            RayJson::CWhere oWhere( "preset" );

            RayJson::Fail( "there is no preset \"" + strName + "\"" );
        }
        return pPreset;
    }
}

IASLib::CObjectNode *RayPresets::ExpandTexture( const IASLib::CJsonNode &oGiven )
{
    const SPreset *pPreset = Named( oGiven );

    if ( pPreset == NULL )
        return NULL;

    std::unique_ptr<IASLib::CJsonNode>  pParsed( ParsePreset( pPreset->strTexture ) );
    IASLib::CObjectNode                *pTexture = dynamic_cast<IASLib::CObjectNode *>( pParsed.get() );
    const IASLib::CObjectNode          *pGiven = dynamic_cast<const IASLib::CObjectNode *>( &oGiven );

    if ( pTexture == NULL || pGiven == NULL )
    {
        RayJson::Fail( "the preset's texture can't be read" );
        return NULL;
    }
    for ( int nIndex = 0; nIndex < pGiven->size(); nIndex++ )
    {
        const std::string &strKey = pGiven->fieldNameString( nIndex );

        if ( strKey != "preset" )
            pTexture->set( strKey, pGiven->fieldValue( nIndex )->clone() );
    }
    pParsed.release();
    return pTexture;
}

IASLib::CArrayNode *RayPresets::ExpandMaterials( const IASLib::CJsonNode &oGiven )
{
    const SPreset *pPreset = Named( oGiven );

    if ( pPreset == NULL )
        return NULL;
    if ( pPreset->strMaterials == NULL )
    {
        RayJson::CWhere oWhere( "preset" );

        RayJson::Fail( std::string( "\"" ) + pPreset->strName + "\" is a texture, not a material: use it as a \"texture\"" );
        return NULL;
    }

    std::unique_ptr<IASLib::CJsonNode>  pParsed( ParsePreset( pPreset->strMaterials ) );
    IASLib::CArrayNode                 *pMaterials = dynamic_cast<IASLib::CArrayNode *>( pParsed.get() );
    const IASLib::CObjectNode          *pGiven = dynamic_cast<const IASLib::CObjectNode *>( &oGiven );

    if ( pMaterials == NULL || pGiven == NULL )
    {
        RayJson::Fail( "the preset's materials can't be read" );
        return NULL;
    }
    for ( int nIndex = 0; nIndex < pGiven->size(); nIndex++ )
    {
        const std::string          &strKey = pGiven->fieldNameString( nIndex );
        const IASLib::CJsonNode    *pValue = pGiven->fieldValue( nIndex );
        BOOL                        bFinish = FALSE;

        if ( strKey == "preset" )
            continue;

        RayJson::CWhere oWhere( strKey.c_str() );

        if ( strKey == "texture" )
        {
            if ( ! pValue->isObject() )
            {
                RayJson::Fail( "must be an object, laid over the preset's texture" );
                return NULL;
            }
            for ( int nMaterial = 0; nMaterial < pMaterials->size(); nMaterial++ )
            {
                IASLib::CJsonNode *pTexture = pMaterials->get( nMaterial )->get( "texture" );

                if ( pTexture && pTexture->isObject() )
                    Overlay( *dynamic_cast<IASLib::CObjectNode *>( pTexture ), *pValue );
            }
            continue;
        }
        for ( int nField = 0; s_astrFinishFields[ nField ]; nField++ )
            bFinish = bFinish || ( strKey == s_astrFinishFields[ nField ] );
        if ( ! bFinish )
        {
            RayJson::Fail( "isn't a field of a preset: beside \"preset\" give \"texture\", \"ambient\", \"diffuse\", "
                           "\"specular\", \"reflective\" or \"transparent\"" );
            return NULL;
        }
        if ( ! pValue->isObject() && ! pValue->isNull() )
        {
            RayJson::Fail( "must be an object, laid over the preset's material of that type, or null to take it out" );
            return NULL;
        }

        int nFound = -1;

        for ( int nMaterial = 0; nMaterial < pMaterials->size() && nFound < 0; nMaterial++ )
        {
            const IASLib::CJsonNode *pType = pMaterials->get( nMaterial )->get( "type" );

            if ( pType && pType->textString() == strKey )
                nFound = nMaterial;
        }
        if ( pValue->isNull() )
        {
            if ( nFound >= 0 )
                pMaterials->remove( nFound );
            continue;
        }
        if ( nFound < 0 )
        {
            IASLib::CObjectNode *pNew = pMaterials->addObject();

            pNew->put( "type", strKey );
            nFound = pMaterials->size() - 1;
        }
        Overlay( *dynamic_cast<IASLib::CObjectNode *>( pMaterials->get( nFound ) ), *pValue );
    }
    pParsed.release();
    return pMaterials;
}
