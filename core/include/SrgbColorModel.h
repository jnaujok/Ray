/*************************************************************************
*   CSrgbColorModel                                                      *
*                                                                        *
*   Clips each channel to 0 through 1 and encodes it with the sRGB       *
*   transfer curve, the encoding image viewers and monitors expect.      *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __SRGBCOLORMODEL_H__
#define __SRGBCOLORMODEL_H__

#include "ColorModel.h"

class CSrgbColorModel : public CColorModel
{
    public:
        virtual CColorModel    *Clone( void ) const { return new CSrgbColorModel( *this ); }
        virtual const char     *GetName( void ) const { return "srgb"; }

        virtual void            ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const;

            /// The sRGB encoding of a linear channel from 0 to 1.
        static FLT              Encode( FLT nLinear );
};

#endif // __SRGBCOLORMODEL_H__
