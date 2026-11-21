/*************************************************************************
*   CSolidTexture                                                        *
*                                                                        *
*   The base of the noise textures (marble, wood, noise): patterns that  *
*   fill space rather than lie on a surface, so an object looks carved   *
*   from a block of the material and needs no surface coordinates.       *
*                                                                        *
*   Each works out a number from 0 to 1 at a point and looks it up in a  *
*   ramp of colors, evenly spaced and blended between. The point is      *
*   first moved by -origin and divided by scale, so scale is the size of *
*   the pattern's features in scene units.                               *
*                                                                        *
*   The noise is Perlin's gradient noise (his 2002 "improved" form, with *
*   its quintic fade and twelve edge gradients), smooth and about -1 to  *
*   1, from a shuffle of 0-255 that the seed picks, so every seed gives  *
*   a different block and the same seed always the same one.             *
*                                                                        *
*   JSON settings every noise texture has: "colors" (two or more; each   *
*   texture has its own default), "scale" (default 1), "origin" (default *
*   [0, 0, 0]), "turbulence" (how much the noise disturbs the pattern),  *
*   "octaves" (finer layers of noise, 1 to 12, default 6), "seed"        *
*   (default 0), "stretch" (default [1, 1, 1]: the scale along x, y and *
*   z on top of "scale", so [40, 1, 1] draws features forty times longer *
*   along x, as brushed metal has).                                      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __SOLIDTEXTURE_H__
#define __SOLIDTEXTURE_H__

#include "Texture.h"

#include <vector>

class CSolidTexture : public CTexture
{
    protected:
        std::vector<CColor>     m_aoColors;
        FLT                     m_nScale;
        CVector                 m_oOrigin;
        FLT                     m_nTurbulence;
        int                     m_nOctaves;
        int                     m_nSeed;
        CVector                 m_oStretch;
            // The seed's shuffle of 0-255, twice over so lookups needn't wrap.
        unsigned char           m_anPermute[ 512 ];

            // Shuffles m_anPermute for m_nSeed.
        void                    Shuffle( void );

            // The pattern's value at a point already moved and scaled
            // (see above); 0 to 1, which the ramp turns into a color.
        virtual FLT             Pattern( FLT nX, FLT nY, FLT nZ ) const = 0;
            // A texture's own fields, besides the ones above.
        virtual void            ExtraToJson( IASLib::CObjectNode &oNode ) const { (void)oNode; }
        virtual BOOL            ExtraFromJson( const IASLib::CJsonNode &oNode ) { (void)oNode; return TRUE; }
        virtual const char *const *ExtraFields( void ) const { return NULL; }

            // Worley's cellular noise at a point: one feature point a unit
            // cell, placed by the seed. nF1 and nF2 are the distances to the
            // nearest and second nearest; nCell is 0 to 1, the same all over
            // the nearest point's cell (a chip of color for flecks).
        void                    Cells( FLT nX, FLT nY, FLT nZ, FLT &nF1, FLT &nF2, FLT &nCell ) const;

    public:
            // A texture with these colors (nColors of them, as r, g, b) and
            // turbulence, and the other settings at their defaults.
                                CSolidTexture( const FLT *anColors, int nColors, FLT nTurbulence );

            /// Gradient noise at a point: smooth, 0 at whole numbers, about
            /// -1 to 1.
        FLT                     Noise( FLT nX, FLT nY, FLT nZ ) const;
            /// Turbulence: the sum of |noise| over m_nOctaves layers, each
            /// twice as fine and half as strong as the last; 0 to about 1.
        FLT                     Turbulence( FLT nX, FLT nY, FLT nZ ) const;
            /// The ramp's color at nAt (0 to 1; outside is clamped).
        CColor                  RampAt( FLT nAt ) const;

        virtual CColor          ColorAt( const STexturePoint &oAt ) const;
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __SOLIDTEXTURE_H__
