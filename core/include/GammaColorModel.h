/*************************************************************************
*   CGammaColorModel                                                     *
*                                                                        *
*   Clips each channel to 0 through 1 and gamma encodes it: the byte is  *
*   255 * channel ^ ( 1 / gamma ), rounded. A gamma of 1 is the clip     *
*   model with rounding.                                                 *
*                                                                        *
*   JSON settings: "gamma" (default 2.2, must be positive).              *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __GAMMACOLORMODEL_H__
#define __GAMMACOLORMODEL_H__

#include "ColorModel.h"

class CGammaColorModel : public CColorModel
{
    protected:
        FLT                     m_nGamma;

    public:
        explicit                CGammaColorModel( FLT nGamma = 2.2 );

        virtual CColorModel    *Clone( void ) const { return new CGammaColorModel( *this ); }
        virtual const char     *GetName( void ) const { return "gamma"; }

        FLT                     GetGamma( void ) const { return m_nGamma; }
        BOOL                    SetGamma( FLT nGamma );

        virtual void            ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const;
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __GAMMACOLORMODEL_H__
