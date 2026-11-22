/*************************************************************************
*   CImageTexture                                                        *
*                                                                        *
*   An image laid over the surface by its surface coordinates: (0, 0) is *
*   the image's bottom left corner and (1, 1) its top right, repeated    *
*   beyond (or with the edge pixels stretched, without "repeat").        *
*   Pixels are blended bilinearly.                                       *
*                                                                        *
*   Images are PNG files (any kind: see PngImage.h), binary PPM (P6) or  *
*   plain PPM (P3) files, as raythrower --ppm writes, or Ray's own raw   *
*   images. A pixel's bytes are taken as the color directly (byte /      *
*   255), matching the default color model; with "srgb" they are         *
*   decoded from the sRGB curve first.                                   *
*                                                                        *
*   A scene names the image file; a saved scene archive carries the file *
*   inside it, and a JSON scene names it relative to the scene file. A   *
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
        std::string                     m_strPath;  // where it was read from, absolute (empty: an archive)
        std::shared_ptr<const std::string> m_pFileData;    // the file as read, to save with the scene
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

            /// Decodes a PNG, JPEG, WebP, PPM or Ray raw image from a file's
            /// contents (strName names it in messages); FALSE, with why in
            /// strError, if it can't be read.
        static BOOL             ParseImage( const std::string &strData, const std::string &strName, SImage &oImage, std::string &strError );
            /// The same, shared: every caller decoding the same bytes (each
            /// copy of a scene, each object using one picture, the images in
            /// glTF models) gets one decoded image, kept while any uses it.
            /// Empty, with why in strError, if it can't be read.
        static std::shared_ptr<const SImage> SharedImage( const std::string &strData, const std::string &strName, std::string &strError );

        virtual CColor          ColorAt( const STexturePoint &oAt ) const;
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const;
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __IMAGETEXTURE_H__
