/*************************************************************************
*   CHdrColorModel                                                       *
*                                                                        *
*   High dynamic range: colors are scaled by an exposure and otherwise   *
*   kept as they are, at any brightness, for a high dynamic range file   *
*   (raythrower --hdr), where an image editor can choose how to show    *
*   them. Files with 8 bits a channel get the same colors clipped at 1   *
*   and encoded with the sRGB curve.                                     *
*                                                                        *
*   JSON settings: "exposure" (default 1, must be positive), the factor  *
*   every color is multiplied by.                                        *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __HDRCOLORMODEL_H__
#define __HDRCOLORMODEL_H__

#include "ColorModel.h"

class CHdrColorModel : public CColorModel
{
    protected:
        FLT                     m_nExposure;

    public:
                                CHdrColorModel( void );

        virtual CColorModel    *Clone( void ) const { return new CHdrColorModel( *this ); }
        virtual const char     *GetName( void ) const { return "hdr"; }

        FLT                     GetExposure( void ) const { return m_nExposure; }
        BOOL                    SetExposure( FLT nExposure );

        virtual void            ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const;
        virtual CColor          ToLinear( const CColor &oColor ) const;
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __HDRCOLORMODEL_H__
