/*************************************************************************
*   CMetalMaterial                                                       *
*                                                                        *
*   A conductor: a mirror whose reflectance, for each of red, green and  *
*   blue, comes from the metal's complex index of refraction n + ik by   *
*   the exact Fresnel equations for a conductor, averaging the two       *
*   polarizations. So gold is yellow face on and whitens toward grazing, *
*   copper is red, aluminum and silver nearly white, and none of them    *
*   is a plain tinted mirror.                                            *
*                                                                        *
*   JSON: "metal" names a preset (gold, silver, copper, aluminum, iron,  *
*   chromium, titanium, platinum); "n" and "k" ([red, green, blue]) give *
*   the index directly and override a preset's; "roughness" (0 to 1,     *
*   default 0) blurs the reflection like a rough mirror's (GGX, from     *
*   where the ray falls in its pixel); "tint" (default white, a color    *
*   or a map) multiplies the result. The presets' n and k are            *
*   representative values for each metal near 650, 550 and 450 nm (from *
*   published optical constants), not measurements of a particular       *
*   sample.                                                              *
*                                                                        *
*   A gold visor (a thin gold film on a clear shell) is this material at *
*   most of the influence beside a "transparent" one for the light that  *
*   gets through.                                                        *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __METAL_MATERIAL__
#define __METAL_MATERIAL__

#include "Material.h"
#include "Globals.h"

#include <string>

class CMetalMaterial : public CMaterial
{
    protected:
        std::string         m_strMetal;         // the preset named, or empty
        FLT                 m_anN[ 3 ];
        FLT                 m_anK[ 3 ];
        BOOL                m_bGivenN;          // n was given (not only from the preset)
        BOOL                m_bGivenK;
        BOOL                m_bTinted;          // a tint was given
        CColorMap           m_oTint;
        CScalarMap          m_nRoughness;

    public:
                            DEFINE_MATERIAL( CMetalMaterial );

        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect );
        virtual void        SetColor( CColor &oColor ) { m_oTint = oColor; }

            /// A conductor's reflectance at this cosine of the angle of
            /// incidence, for unpolarized light.
        static FLT          Reflectance( FLT nCos, FLT nN, FLT nK );
            /// A preset's n and k; FALSE if there is none by that name.
        static BOOL         Preset( const std::string &strName, FLT anN[ 3 ], FLT anK[ 3 ] );
};

#endif
