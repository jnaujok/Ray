/*************************************************************************
*   CLight header                                                        *
*                                                                        *
* This header defines the base light class for use in deriving other,    *
* more specific lights like point-lights and spot-lights.                *
*                                                                        *
* Options every light can take (all off unless given, so a light without *
* them shines exactly as before):                                        *
*                                                                        *
*   "attenuation"   how the light weakens with distance: "none" (the     *
*                   default: the same at every distance), "inverse"      *
*                   (as 1/d) or "inverseSquare" (as 1/d^2, as real light *
*                   does); "distance" (default 1) is where it is its     *
*                   full color. Not for sunlight.                        *
*   "tint"          a color the light is multiplied by at the point it   *
*                   lights: a color, or a texture read at that point in  *
*                   the scene (a solid texture: marble, noise, cells,    *
*                   caustics), so what light arrives depends on where.   *
*   "intensity"     a number the light is multiplied by there (default   *
*                   1): a number, or a texture read the same way.        *
*   "filter"        an image or pattern the light shines through, like a *
*                   slide in a projector or a gobo in a stage light: an  *
*                   object with a "texture" (any texture or preset) and  *
*                   "size" (default 1). A spot light projects it across  *
*                   its cone, sunlight straight along its direction      *
*                   (size: scene units the texture's unit square spans), *
*                   a rectangle light along its face, and a point or     *
*                   sphere light all around it. Image textures read its  *
*                   (u, v); solid textures read a point in the light's   *
*                   picture, so a caustics pattern needs no image.       *
*                                                                        *
* Spot lights, and rectangle lights given an "angle", also take a beam   *
* "profile" (see ProfileFactor).                                         *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __LIGHT_H__
#define __LIGHT_H__

#include "Objects.h"
#include "Texture.h"
#include "Maps.h"

#include <memory>
#include <vector>

    // How far a shadow ray toward sunlight (or the sky) goes, in scene units:
    // far past anything in a scene.
#define RAY_FAR_AWAY    1.0e6

class CLight : public CObjects
{
    public:
        enum EAttenuation { ATTENUATION_NONE = 0, ATTENUATION_INVERSE, ATTENUATION_INVERSE_SQUARE };
        enum EProfile { PROFILE_SMOOTHSTEP = 0, PROFILE_LINEAR, PROFILE_COSINE, PROFILE_GAUSSIAN, PROFILE_FRESNEL,
                        PROFILE_TABLE };

    protected:
        CColor          m_oColor;
        CVector         m_oPosition;

        int             m_nAttenuation;
        FLT             m_nDistance;            // where an attenuated light is its full color
        std::shared_ptr<const CTexture> m_pFilter;
        FLT             m_nFilterSize;
            // The beam's shape across a cone (spot lights; rectangle lights
            // with an angle).
        int             m_nProfile;
        FLT             m_nExponent;            // cosine: cos^exponent of the angle from the axis
        FLT             m_nSpread;              // gaussian: its width in degrees (0: a third of the angle)
        FLT             m_nHotspot;             // fresnel: the bright center's angle in degrees (0: a third of the angle)
        FLT             m_nRim;                 // fresnel: the beam's brightness outside the center, 0 to 1
        std::vector<FLT> m_anTable;             // table: brightness at even steps from the axis to the edge
            // The light's color and strength at the point it lights: 1 unless
            // given (a constant, or a texture read at that point).
        CColorMap       m_oTint;
        CScalarMap      m_nIntensity;

            // The beam's brightness at nCos (the cosine of the angle from the
            // axis), in a cone of nAngle degrees whose edge fades over the
            // last nFalloff degrees: 0 outside, up to 1. The default profile,
            // smoothstep, is the fading edge alone (the spot light's own).
        FLT             ProfileFactor( FLT nCos, FLT nAngle, FLT nFalloff ) const;
            // Whether the tint is a constant other than white.
        BOOL            HasTint( void ) const;
            // Where the filter is looked up for light reaching oPoint along
            // oToLight (from the point toward the light). Lights override it
            // with their own projection; the default looks all around.
        virtual void    FilterAt( const CVector &oPoint, const CVector &oToLight, STexturePoint &oAt ) const;
            // The options' JSON: their fields go into oNode (only those not
            // at their defaults), or are read from it.
        void            OptionsToJson( IASLib::CObjectNode &oNode, BOOL bProfile ) const;
        BOOL            OptionsFromJson( const IASLib::CJsonNode &oNode, BOOL bAttenuation, BOOL bProfile );
            // Checks oNode has only the light's own fields (astrOwn) and the
            // options it takes.
        static BOOL     OnlyLightFields( const IASLib::CJsonNode &oNode, const char *const *astrOwn, BOOL bAttenuation,
                                         BOOL bProfile );
    public:
                        CLight( void );
                        CLight( FLT nRed, FLT nGreen, FLT nBlue, FLT nX = 0.0, FLT nY = 0.0, FLT nZ = 0.0 );
        virtual        ~CLight( void );

        virtual CVector GetPosition( void ) { return m_oPosition; }
        virtual CColor  GetColor( void ) { return m_oColor; }
        virtual CColor  GetShade( CVector oDirection ) = 0;
            /// The light reaching oPoint along oToLight (from the point to
            /// the light, or to the point of an area light it is shaded
            /// from): GetShade, weakened with distance and colored by the
            /// filter if the light has them.
        CColor          ShadeAt( const CVector &oPoint, const CVector &oToLight );
            /// The vector from oFrom to the light, which a shadow ray follows
            /// (its length is how far the ray goes): to the light's position,
            /// or, for sunlight, RAY_FAR_AWAY back along its direction.
        virtual CVector ToLight( CVector &oFrom ) { return m_oPosition - oFrom; }
            /// How many points of the light a surface is shaded from: 0 for a
            /// point light (its position), or an area light's samples.
        virtual int     GetSampleCount( void ) { return 0; }
            /// The point of an area light that the point (nU, nV) of the unit
            /// square picks, as seen from oFrom. FALSE if oFrom gets no light
            /// from it (behind a one-sided light).
        virtual BOOL    SampleAt( CVector &oFrom, FLT nU, FLT nV, CVector &oPoint )
                        { (void)oFrom; (void)nU; (void)nV; oPoint = m_oPosition; return TRUE; }
        virtual BOOL    IsALight( void ) { return TRUE; }
#ifdef RAY_GUI
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder ) = 0;
#endif
};

#endif // __LIGHT_H__
