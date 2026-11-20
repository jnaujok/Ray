/*************************************************************************
*   CEnvironment                                                         *
*                                                                        *
*   What a ray that leaves the scene sees: the sky, instead of the flat  *
*   background color. Camera rays that miss everything, and reflected    *
*   and refracted rays that escape, all take their color from it, so it  *
*   shows behind the scene, in mirrors, and through glass.               *
*                                                                        *
*   Two kinds ("type"):                                                  *
*                                                                        *
*       sky     a gradient by height: "zenith" overhead, "horizon" at    *
*               the horizon, "ground" below it. Up is +y. Its colors     *
*               blend by the height of the direction (its y, from -1 to  *
*               1) raised to "exponent" (default 0.5: smaller keeps the  *
*               horizon's color in a thinner band).                      *
*       image   an equirectangular (latitude-longitude) picture of the   *
*               whole surroundings: "file", a PNG, PPM, Ray raw or       *
*               Radiance HDR (.hdr, RGBE) image, twice as wide as it is  *
*               tall. Its top row is straight up, its bottom straight    *
*               down, and the middle of the picture lies along +z,       *
*               turned about +y by "rotation" degrees. An 8-bit image's  *
*               bytes are taken as linear colors (byte / 255), or with   *
*               "srgb" decoded from the sRGB curve; an HDR's colors are  *
*               linear and may be brighter than 1.                       *
*                                                                        *
*   Both take "intensity" (default 1), which scales the color, and       *
*   "samples" (default 0). With samples, the environment also lights     *
*   diffuse surfaces (image-based lighting). The light a surface would   *
*   get from the whole sky over it, for every way it could face, is      *
*   worked out once when the scene is read, into a small map             *
*   (IRRADIANCE_WIDTH x IRRADIANCE_HEIGHT), so it is smooth, with no     *
*   grain, even from a sky with a sun far brighter than white. Then at   *
*   each point "samples" shadow rays go toward the sky (spread by the    *
*   cosine, from the ray's sample point, CRay::SamplePoint, so a         *
*   stratified sampler covers the sky evenly over a pixel's rays), and   *
*   the share that get out scales that light: corners, crevices and the  *
*   ground under things are darker. It costs that many shadow rays at    *
*   every point a ray hits.                                              *
*                                                                        *
*   The image file goes into a saved scene archive with the scene; a     *
*   copy of the scene sent to a task carries the file's bytes in "data"  *
*   (see RayJson::CEmbedData), so engines need no files.                 *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__

#include "Globals.h"
#include "Color.h"
#include "Vector.h"

#include <memory>
#include <string>
#include <vector>

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

class CEnvironment
{
    public:
        enum EType { TYPE_SKY = 0, TYPE_IMAGE = 1 };

        struct SImage
        {
            int                 nWidth;
            int                 nHeight;
            std::vector<float>  anPixels;       // linear RGB, top row first
        };

    protected:
        enum { IRRADIANCE_WIDTH = 32, IRRADIANCE_HEIGHT = 16, RADIANCE_WIDTH = 64, RADIANCE_HEIGHT = 32 };

        EType                   m_eType;
        CColor                  m_oZenith;
        CColor                  m_oHorizon;
        CColor                  m_oGround;
        FLT                     m_nExponent;
        FLT                     m_nIntensity;
        FLT                     m_nRotation;    // degrees about +y
        BOOL                    m_bSrgb;
        int                     m_nSamples;
        std::string             m_strFile;      // as the scene gave it
        std::string             m_strPath;      // where it was read from (empty: an archive or a scene copy)
        std::shared_ptr<const std::string> m_pFileData;
        std::shared_ptr<const SImage> m_pImage;
            // The light from the sky for each way a surface can face, over pi
            // (as a light's color is), laid out as the image is; built by
            // BuildIrradiance when there are samples.
        std::shared_ptr<const std::vector<float> > m_pIrradiance;

        CColor                  Pixel( int nX, int nY ) const;
        CColor                  ImageColor( FLT nX, FLT nY, FLT nZ ) const;
            /// The unrotated direction that the point (nU, nV) of the image
            /// looks along (see ImageCoords).
        static void             DirectionAt( FLT nU, FLT nV, FLT anDirection[ 3 ] );
        void                    BuildIrradiance( void );

    public:
                                CEnvironment( void );

            /// The color seen looking along oDirection (any length but 0).
        CColor                  ColorIn( const CVector &oDirection ) const;
            /// How many directions the environment lights each point from (0:
            /// it lights nothing; it is only seen).
        int                     GetSamples( void ) const { return m_nSamples; }
            /// The light from the whole sky reaching a surface whose normal is
            /// oNormal (any length but 0), over pi: the color a light would
            /// need to light it as much from straight overhead. Black if the
            /// environment has no samples.
        CColor                  IrradianceIn( const CVector &oNormal ) const;
        EType                   GetType( void ) const { return m_eType; }

            /// Where in the image a direction looks: u from 0 at the left edge
            /// to 1 at the right, v from 0 at the top to 1 at the bottom.
        void                    ImageCoords( FLT nX, FLT nY, FLT nZ, FLT &nU, FLT &nV ) const;

            /// Decodes a Radiance HDR (RGBE) image, flat or run-length encoded,
            /// stored top row first and left to right ("-Y height +X width").
            /// FALSE, with why in strError, if it can't be read.
        static BOOL             ParseHdr( const std::string &strData, int &nWidth, int &nHeight, std::vector<float> &anPixels,
                                          std::string &strError );

        void                    ToJson( IASLib::CObjectNode &oNode ) const;
        BOOL                    FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __ENVIRONMENT_H__
