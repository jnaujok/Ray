/*************************************************************************
*   CReinhardColorModel                                                  *
*                                                                        *
*   Tone mapping: instead of clipping bright colors at white, compresses *
*   them with Reinhard's operator, then encodes with the sRGB curve.     *
*   The operator works on luminance, so hues are kept:                   *
*                                                                        *
*       L  = luminance of ( color * exposure )                           *
*       Ld = L * ( 1 + L / white^2 ) / ( 1 + L )                         *
*       out = color * exposure * Ld / L                                  *
*                                                                        *
*   With no white point, Ld = L / ( 1 + L ): nothing quite reaches white. *
*   With one, luminance "white" and above maps to white.                 *
*                                                                        *
*   JSON settings: "exposure" (default 1, must be positive) and "white"  *
*   (default none; if given, must be positive).                          *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __REINHARDCOLORMODEL_H__
#define __REINHARDCOLORMODEL_H__

#include "ColorModel.h"

class CReinhardColorModel : public CColorModel
{
    protected:
        FLT                     m_nExposure;
        FLT                     m_nWhite;           // 0: no white point.

    public:
                                CReinhardColorModel( void );

        virtual CColorModel    *Clone( void ) const { return new CReinhardColorModel( *this ); }
        virtual const char     *GetName( void ) const { return "reinhard"; }

        FLT                     GetExposure( void ) const { return m_nExposure; }
        BOOL                    SetExposure( FLT nExposure );
        FLT                     GetWhite( void ) const { return m_nWhite; }
            /// 0 removes the white point.
        BOOL                    SetWhite( FLT nWhite );

        virtual void            ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const;
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __REINHARDCOLORMODEL_H__
