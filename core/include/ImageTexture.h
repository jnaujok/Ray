/*************************************************************************
*   CImageTexture                                                        *
*                                                                        *
*   An image laid over the surface by its surface coordinates: (0, 0) is *
*   the image's bottom left corner and (1, 1) its top right, repeated    *
*   beyond (or with the edge pixels stretched, without "repeat").        *
*   Pixels are blended bilinearly.                                       *
*                                                                        *
*   Images are binary PPM (P6) or plain PPM (P3) files, as raythrower    *
*   --ppm writes, or Ray's own raw images. A pixel's bytes are taken as  *
*   the color directly (byte / 255), matching the default color model;   *
*   with "srgb" they are decoded from the sRGB curve first.              *
*                                                                        *
*   A scene file names the image file, relative to the scene file. A     *
*   copy of the scene sent to a task carries the pixels instead (see     *
*   RayJson::CEmbedData), so engines need no files.                      *
*                                                                        *
*   JSON settings: "file" (required, unless "pixels" are given), "scale" *
*   ([u, v] repeats; default [1, 1]), "offset" ([u, v]; default [0, 0]), *
*   "repeat" (default true), "srgb" (default false).                     *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __IMAGETEXTURE_H__
#define __IMAGETEXTURE_H__

#include "Texture.h"

#include <memory>
#include <string>
#include <vector>

class CImageTexture : public CTexture
{
    public:
        struct SImage
        {
            int                         nWidth;
            int                         nHeight;
            std::vector<unsigned char>  anPixels;   // RGB, top row first
        };

    protected:
        std::string                     m_strFile;  // as the scene gave it
        std::string                     m_strPath;  // where it was read from, absolute
        std::shared_ptr<const SImage>   m_pImage;   // shared by copies
        FLT                             m_anScale[ 2 ];
        FLT                             m_anOffset[ 2 ];
        BOOL                            m_bRepeat;
        BOOL                            m_bSrgb;

        CColor                  Pixel( int nX, int nY ) const;

    public:
                                CImageTexture( void );

        virtual CTexture       *Clone( void ) const { return new CImageTexture( *this ); }
        virtual const char     *GetName( void ) const { return "image"; }

            /// Reads a PPM or Ray raw image; FALSE, with why in strError, if
            /// it can't be read.
        static BOOL             ReadImage( const std::string &strPath, SImage &oImage, std::string &strError );

        virtual CColor          ColorAt( const STexturePoint &oAt ) const;
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __IMAGETEXTURE_H__
