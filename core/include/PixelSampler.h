/*************************************************************************
*   CPixelSampler                                                        *
*                                                                        *
*   Decides which primary rays the camera casts for each pixel, and how  *
*   their colors become the pixel's color. A camera owns one sampler;    *
*   attach a different one to change how an image is sampled, for        *
*   example for anti-aliasing. The pixel colors it works out are linear  *
*   and of any brightness; the camera's color model turns them into      *
*   bytes when the sampler hands them to the CPixelWriter.               *
*                                                                        *
*   The image is traced in tiles, each by one task, which runs the      *
*   camera's sampler for its tile like this:                             *
*                                                                        *
*       nRays = Begin( x, y, width, height );   the tile, in pixels      *
*       while ( NextRay( scene, ray ) )   trace the ray                  *
*       PutSample( writer, ... )          once for every traced ray,     *
*                                         straight after tracing it      *
*                                         (CTileTracer does), which lets *
*                                         a sampler decide on its next   *
*                                         ray from the colors so far     *
*       End();                                                           *
*                                                                        *
*   Rays, tags and pixels are in image coordinates. A sampler only       *
*   writes pixels inside its tile; a ray it shares with pixels outside   *
*   (a corner) is traced again by the neighboring tile, and comes out    *
*   the same, so tiles meet without a seam.                              *
*                                                                        *
*   NextRay labels each ray with a tag (CRay::GetPixelX/GetPixelY),      *
*   which comes back with its color in PutSample. The tag can mean       *
*   whatever the sampler likes, as long as it fits in a 32-bit unsigned  *
*   value; the tracing tasks never look at it.                           *
*                                                                        *
*   To add a sampler, derive from this class and add it to the table in  *
*   PixelSampler.cpp so it can be chosen by name.                        *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __PIXELSAMPLER_H__
#define __PIXELSAMPLER_H__

#include "Globals.h"

class CScene;
class CRay;
class CColor;
class CPixelWriter;

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

class CPixelSampler
{
    public:
            // One entry of the table of samplers that can be chosen by name.
        struct SType
        {
            const char         *strName;
            const char         *strDescription;
            CPixelSampler    *(*pfnCreate)( void );
        };

        virtual            ~CPixelSampler( void ) {}

            /// A new sampler of the same kind and settings, without any trace
            /// in progress.
        virtual CPixelSampler  *Clone( void ) const = 0;
            /// The name it is chosen by, such as "corners".
        virtual const char     *GetName( void ) const = 0;

            /// Starts sampling a tile of the image: nWidth by nHeight pixels
            /// from (nLeft, nTop). Returns the number of rays NextRay will make.
        virtual long long       Begin( int nLeft, int nTop, int nWidth, int nHeight ) = 0;
            /// Starts sampling a whole image as one tile.
        long long               BeginImage( int nWidth, int nHeight ) { return Begin( 0, 0, nWidth, nHeight ); }
            /// Makes the next primary ray. Returns FALSE when there are no more.
        virtual BOOL            NextRay( CScene &oScene, CRay &oRay ) = 0;
            /// Takes the color traced for the ray tagged (nTagX, nTagY), and
            /// writes any pixels it completes with oWriter.
        virtual void            PutSample( CPixelWriter &oWriter, int nTagX, int nTagY, const CColor &oColor ) = 0;
            /// Ends the trace, finished or not, and frees what it needed.
        virtual void            End( void ) {}

            /// Writes the sampler's settings, if it has any, into a scene
            /// file's JSON object for it (which already holds its "type").
        virtual void            ToJson( IASLib::CObjectNode &oNode ) const { (void)oNode; }
            /// Reads the settings back from the object, or from a plain name
            /// string. Returns FALSE if one is not valid; the default accepts
            /// no settings at all.
        virtual BOOL            FromJson( const IASLib::CJsonNode &oNode );

            /// A new sampler by name, or NULL if there is none by that name.
        static CPixelSampler   *Create( const char *strName );
            /// The samplers that can be chosen by name. The first is the default.
        static const SType     *GetTypes( int &nCount );
};

#endif // __PIXELSAMPLER_H__
