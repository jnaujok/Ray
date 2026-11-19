/*************************************************************************
*   CClipColorModel                                                      *
*                                                                        *
*   The default color model, and the one from 1997: each channel is      *
*   clipped at 1, scaled to 255 and truncated, with no gamma encoding.   *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __CLIPCOLORMODEL_H__
#define __CLIPCOLORMODEL_H__

#include "ColorModel.h"

class CClipColorModel : public CColorModel
{
    public:
        virtual CColorModel    *Clone( void ) const { return new CClipColorModel( *this ); }
        virtual const char     *GetName( void ) const { return "clip"; }

        virtual void            ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const;
};

#endif // __CLIPCOLORMODEL_H__
