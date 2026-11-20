/*************************************************************************
*   CMarbleTexture                                                       *
*                                                                        *
*   Marble: bands across "axis" (default [1, 0, 0]), one each scale,     *
*   bent by turbulence (default 2.5) so they wander. The value is        *
*   | sin( pi ( p.axis + turbulence * T(p) ) ) |, which falls sharply to *
*   0 at the edge between bands, so the ramp's first color draws thin    *
*   veins and its last the stone between them.                           *
*                                                                        *
*   JSON settings: those of CSolidTexture, and "axis". Default colors:   *
*   white marble with gray veins.                                        *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __MARBLETEXTURE_H__
#define __MARBLETEXTURE_H__

#include "SolidTexture.h"

class CMarbleTexture : public CSolidTexture
{
    protected:
        CVector                 m_oAxis;

        virtual FLT             Pattern( FLT nX, FLT nY, FLT nZ ) const;
        virtual void            ExtraToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            ExtraFromJson( const IASLib::CJsonNode &oNode );
        virtual const char *const *ExtraFields( void ) const;

    public:
                                CMarbleTexture( void );

        virtual CTexture       *Clone( void ) const { return new CMarbleTexture( *this ); }
        virtual const char     *GetName( void ) const { return "marble"; }
};

#endif // __MARBLETEXTURE_H__
