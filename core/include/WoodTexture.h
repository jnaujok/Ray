/*************************************************************************
*   CWoodTexture                                                         *
*                                                                        *
*   Wood: rings about the line through origin along "axis" (default      *
*   [0, 1, 0]), one each scale, wobbled by turbulence (default 0.3).     *
*   Each ring runs through the ramp from 0 to 1 and starts over, so the  *
*   last color is the dark edge of a ring.                               *
*                                                                        *
*   JSON settings: those of CSolidTexture, and "axis". Default colors:   *
*   pine.                                                                *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __WOODTEXTURE_H__
#define __WOODTEXTURE_H__

#include "SolidTexture.h"

class CWoodTexture : public CSolidTexture
{
    protected:
        CVector                 m_oAxis;

        virtual FLT             Pattern( FLT nX, FLT nY, FLT nZ ) const;
        virtual void            ExtraToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            ExtraFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ExtraFields( void ) const;

    public:
                                CWoodTexture( void );

        virtual CTexture       *Clone( void ) const { return new CWoodTexture( *this ); }
        virtual const char     *GetName( void ) const { return "wood"; }
};

#endif // __WOODTEXTURE_H__
