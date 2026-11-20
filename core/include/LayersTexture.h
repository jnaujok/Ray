/*************************************************************************
*   CLayersTexture                                                       *
*                                                                        *
*   The layer lines of a 3D-printed part: a solid texture of rounded     *
*   ridges, one each "scale" (the layer height, default 0.1) along       *
*   "axis" (default [0, 0, 1], the print direction of a model file, most *
*   of which stand on z). Each layer is a rounded bead: the pattern is 0 *
*   at the seams between layers and rises along a parabola to 1 in the   *
*   middle of each, so as a color it darkens the seams, and as a bump    *
*   map's heights it gives each layer its rounded edge.                  *
*                                                                        *
*   A mesh loaded from a file looks its textures up in the file's own    *
*   coordinates (millimeters, for a printable model), so a layer height  *
*   of 0.1 is 0.1 mm and the layers follow the print axis however the    *
*   model is turned or scaled in the scene. Elsewhere, scale is in scene *
*   units.                                                               *
*                                                                        *
*   "turbulence" (default 0) wobbles the layers slowly, as an uneven     *
*   printer would; its noise is about twenty layers across.              *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __LAYERSTEXTURE_H__
#define __LAYERSTEXTURE_H__

#include "SolidTexture.h"

class CLayersTexture : public CSolidTexture
{
    protected:
        CVector                 m_oAxis;

        virtual FLT             Pattern( FLT nX, FLT nY, FLT nZ ) const;
        virtual void            ExtraToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            ExtraFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ExtraFields( void ) const;

    public:
                                CLayersTexture( void );

        virtual CTexture       *Clone( void ) const { return new CLayersTexture( *this ); }
        virtual const char     *GetName( void ) const { return "layers"; }

            /// The rounded bead's height at a fraction of the way through a
            /// layer (0 to 1): 0 at both seams, 1 in the middle.
        static FLT              Bead( FLT nFraction );
};

#endif // __LAYERSTEXTURE_H__
