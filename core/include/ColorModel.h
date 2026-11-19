/*************************************************************************
*   CColorModel                                                          *
*                                                                        *
*   The last step of making an image: turns the color a sampler worked   *
*   out for a pixel, a linear color of any brightness, into the bytes    *
*   written to the image file. A camera owns one color model, the way a  *
*   camera holds a film; attach a different one to change how colors    *
*   are clipped, gamma encoded, or tone mapped.                          *
*                                                                        *
*   To add a color model, derive from this class and add it to the table *
*   in ColorModel.cpp so it can be chosen by name.                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __COLORMODEL_H__
#define __COLORMODEL_H__

#include "Globals.h"

class CColor;

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

class CColorModel
{
    public:
            // One entry of the table of color models that can be chosen by name.
        struct SType
        {
            const char         *strName;
            const char         *strDescription;
            CColorModel      *(*pfnCreate)( void );
        };

        virtual            ~CColorModel( void ) {}

            /// A new color model of the same kind and settings.
        virtual CColorModel    *Clone( void ) const = 0;
            /// The name it is chosen by, such as "srgb".
        virtual const char     *GetName( void ) const = 0;

            /// The bytes to write for a pixel of this linear color.
        virtual void            ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const = 0;

            /// Writes the model's settings, if it has any, into a scene file's
            /// JSON object for it (which already holds its "type").
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const { (void)oNode; }
            /// Reads the settings back from the object, or from a plain name
            /// string. Returns FALSE if one is not valid; the default accepts
            /// no settings at all.
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );

            /// A new color model by name, or NULL if there is none by that name.
        static CColorModel     *Create( const char *strName );
            /// The color models that can be chosen by name. The first is the default.
        static const SType     *GetTypes( int &nCount );

    protected:
            /// Clamps a channel to 0 through 1 (a NaN becomes 0).
        static FLT              Clamp( FLT nValue );
            /// A channel from 0 to 1 as the nearest byte.
        static BYTE             ToByte( FLT nValue );
};

#endif // __COLORMODEL_H__
