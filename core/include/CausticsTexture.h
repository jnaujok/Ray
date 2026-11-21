/*************************************************************************
*   CCausticsTexture                                                     *
*                                                                        *
*   The rippling web of bright lines that light makes under water,       *
*   where waves on the surface focus it: the edges between Worley cells  *
*   (see CCellsTexture), their sides bent by noise, in one or more       *
*   layers turned and scaled against each other. The value is 1 on a     *
*   line and falls to 0 a "width" (a fraction of a cell) away from it,   *
*   so the ramp's first color is the light between the lines and its     *
*   last the lines themselves. Projected from a light as its "filter",   *
*   it throws the pattern over everything the light reaches.             *
*                                                                        *
*   JSON settings: those of CSolidTexture, "width" (default 0.12) and    *
*   "layers" (1 to 3, default 2). Default turbulence 0.6; default colors *
*   a dim blue-gray to bright white (brighter than 1, so the lines add   *
*   light, as a focused caustic does).                                   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __CAUSTICSTEXTURE_H__
#define __CAUSTICSTEXTURE_H__

#include "SolidTexture.h"

class CCausticsTexture : public CSolidTexture
{
    protected:
        FLT                     m_nWidth;
        int                     m_nLayers;

            // One layer's lines at a point: 1 on a line, 0 away from it.
        FLT                     Lines( FLT nX, FLT nY, FLT nZ ) const;
        virtual FLT             Pattern( FLT nX, FLT nY, FLT nZ ) const;
        virtual void            ExtraToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            ExtraFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ExtraFields( void ) const;

    public:
                                CCausticsTexture( void );

        virtual CTexture       *Clone( void ) const { return new CCausticsTexture( *this ); }
        virtual const char     *GetName( void ) const { return "caustics"; }
};

#endif // __CAUSTICSTEXTURE_H__
