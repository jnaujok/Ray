/*************************************************************************
*   CNoiseTexture                                                        *
*                                                                        *
*   The noise itself, for granite, stone or clouds. With turbulence 0    *
*   (the default), smooth layered noise: 0.5 + 0.5 times the sum over    *
*   the octaves. With more, turbulence times CSolidTexture::Turbulence,  *
*   which has sharp creases.                                             *
*                                                                        *
*   JSON settings: those of CSolidTexture. Default colors: black to      *
*   white.                                                               *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __NOISETEXTURE_H__
#define __NOISETEXTURE_H__

#include "SolidTexture.h"

class CNoiseTexture : public CSolidTexture
{
    protected:
        virtual FLT             Pattern( FLT nX, FLT nY, FLT nZ ) const;

    public:
                                CNoiseTexture( void );

        virtual CTexture       *Clone( void ) const { return new CNoiseTexture( *this ); }
        virtual const char     *GetName( void ) const { return "noise"; }
};

#endif // __NOISETEXTURE_H__
