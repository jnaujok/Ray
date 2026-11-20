/*************************************************************************
*   CLens                                                                *
*                                                                        *
*   A real camera lens: a series of spherical glass surfaces, traced     *
*   one after another from the sensor out to the scene. Rays that hit    *
*   the barrel or the aperture stop are blocked, which gives natural     *
*   vignetting and depth of field, and the spherical surfaces give the   *
*   lens's own aberrations and distortion.                               *
*                                                                        *
*   A lens prescription lists the surfaces from the front (the scene     *
*   side) to the back, as lens patents do. Each surface has:             *
*                                                                        *
*       radius      of curvature in mm; positive when the center of      *
*                   curvature is behind the surface (toward the sensor); *
*                   0 for a flat surface                                 *
*       thickness   distance along the axis to the next surface, in mm   *
*                   (the last surface's is ignored: the sensor's place   *
*                   is worked out by focusing)                           *
*       ior         index of refraction of what follows the surface      *
*                   (1 for air)                                          *
*       aperture    the surface's semi-aperture: its radius in mm, past  *
*                   which a ray hits the barrel                          *
*       stop        whether this is the aperture stop (flat); its        *
*                   opening follows the f-number                         *
*       abbe        the Abbe number of the glass that follows (0: no     *
*                   dispersion)                                          *
*                                                                        *
*   Chromatic aberration: glass bends short wavelengths more than long   *
*   ones. With Abbe numbers, each glass's index is fitted to a Cauchy    *
*   curve, n = A + B / wavelength^2, through its index at the d line and *
*   its Abbe number, and the red, green and blue channels are traced at  *
*   the C (656.3 nm), d (587.6 nm) and F (486.1 nm) lines. Green is the  *
*   index as given, so focus and exposure are worked out for it.         *
*                                                                        *
*   In the lens's own frame, in millimeters, the front surface is at     *
*   z = 0 and light travels toward +z, to the sensor. The camera puts    *
*   the front surface at its position, looking at its look-at point.     *
*                                                                        *
*   Each ray goes from a point on the sensor (the image is upside down   *
*   and mirrored there, as in a real camera) to a point on the rear      *
*   element. The camera picks that point from where the ray falls in its *
*   pixel, shifted by a hash of the pixel and the seed, so a sampler     *
*   that spreads its rays evenly over each pixel spreads them evenly     *
*   over the lens too, and the same scene always gives the same image.   *
*   One ray a pixel is grainy: use a sampler that traces many, evenly    *
*   weighted ("stratified").                                             *
*                                                                        *
*   Exposure is set so the center of the image is as bright as it would  *
*   be through a pinhole: the share of rays the lens lets through at the *
*   center of the sensor is worked out when the lens is prepared, and    *
*   pixels are scaled by its inverse.                                    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __LENS_H__
#define __LENS_H__

#include "Globals.h"

#include <cstdint>
#include <string>
#include <vector>

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

class CLens
{
    public:
        struct SSurface
        {
            FLT                 nRadius;
            FLT                 nThickness;
            FLT                 nIor;
            FLT                 nAperture;
            BOOL                bStop;
            FLT                 nAbbe;          // 0: the glass doesn't disperse
        };

            // The channel a ray is traced in: every channel at the d line, or
            // one of red, green and blue at its own wavelength.
        enum EChannel { CHANNEL_ALL = 0, CHANNEL_RED = 1, CHANNEL_GREEN = 2, CHANNEL_BLUE = 3 };

            // One entry of the table of lenses that can be named in a scene.
        struct SPrescription
        {
            const char         *strName;
            const char         *strDescription;
            const SSurface     *aoSurfaces;
            int                 nSurfaces;
        };

    protected:
        std::string             m_strPrescription;  // the built-in lens's name, or empty
        std::vector<SSurface>   m_aoSurfaces;
        FLT                     m_nFNumber;         // 0: wide open
        FLT                     m_nFocusDistance;   // in scene units from the front surface; 0: the look-at point
        FLT                     m_nSensorWidth;     // mm
        FLT                     m_nMillimetersPerUnit;
        int                     m_nSeed;
        BOOL                    m_bChromatic;       // trace the channels apart, if any glass disperses

            // Worked out by Prepare().
        std::vector<FLT>        m_anVertexZ;        // each surface's place on the axis
        FLT                     m_nFocalLength;
        FLT                     m_nWideOpen;        // the smallest f-number
        FLT                     m_nStopRadius;      // the stop's opening at the f-number
        FLT                     m_nSensorZ;
        FLT                     m_nFocusedAt;       // the focus distance used, in scene units
        FLT                     m_nExposure;
        BOOL                    m_bPrepared;

            // Traces a ray through the surfaces, forward (toward the
            // sensor) or backward. The ray is changed in place; FALSE if it
            // is blocked or totally internally reflected.
        BOOL                    Trace( FLT anOrigin[ 3 ], FLT anDirection[ 3 ], BOOL bForward, FLT nStopRadius,
                                       int nChannel = CHANNEL_ALL ) const;
            // The index of what follows surface nIndex (or air, for -1) in a channel.
        FLT                     IorAt( int nIndex, int nChannel ) const;
            // Where a paraxial ray from an on-axis point nObjectZ (or from
            // infinity, if bInfinity) crosses the axis behind the lens.
        BOOL                    ImageZ( FLT nObjectZ, BOOL bInfinity, FLT &nImageZ, int nChannel = CHANNEL_ALL ) const;

    public:
                                CLens( void );

        CLens                  *Clone( void ) const { return new CLens( *this ); }

            /// Uses a built-in prescription; FALSE if there is none by that name.
        BOOL                    SetPrescription( const char *strName );
        void                    SetSurfaces( const std::vector<SSurface> &aoSurfaces );
        void                    SetFNumber( FLT nFNumber ) { m_nFNumber = nFNumber; m_bPrepared = FALSE; }
        void                    SetFocusDistance( FLT nDistance ) { m_nFocusDistance = nDistance; m_bPrepared = FALSE; }

            /// Works out the lens's focal length, widest f-number, where the
            /// sensor goes to focus at the focus distance (nDefaultFocus, in
            /// scene units, if none was given), and the exposure. FALSE, with
            /// a reason in strError, if the lens can't be used so.
        BOOL                    Prepare( FLT nDefaultFocus, std::string &strError );

            /// The ray for a point on the image (nScreenX from -1 at the left
            /// to 1 at the right, nScreenY from -1 at the bottom to 1 at the
            /// top; nAspect is width over height), in the lens frame and in
            /// millimeters, leaving the front of the lens toward the scene.
            /// nLensU and nLensV, from 0 to 1, pick the point on the rear
            /// element (an even spread of them covers it evenly). FALSE if
            /// the lens blocks the ray.
        BOOL                    RayFor( FLT nScreenX, FLT nScreenY, FLT nAspect, FLT nLensU, FLT nLensV,
                                        FLT anOrigin[ 3 ], FLT anDirection[ 3 ], int nChannel = CHANNEL_ALL ) const;
            /// Whether the channels are traced apart: some glass has an Abbe
            /// number, and chromatic aberration isn't turned off.
        BOOL                    IsChromatic( void ) const;
            /// The focal length for a channel's wavelength (after Prepare).
        FLT                     FocalLengthIn( int nChannel ) const;
        int                     GetSeed( void ) const { return m_nSeed; }

        FLT                     GetFocalLength( void ) const { return m_nFocalLength; }
        FLT                     GetWideOpen( void ) const { return m_nWideOpen; }
        FLT                     GetFNumber( void ) const { return ( m_nFNumber > 0.0 ) ? m_nFNumber : m_nWideOpen; }
        FLT                     GetFocusedAt( void ) const { return m_nFocusedAt; }
            /// From the rear surface to the sensor, in mm.
        FLT                     GetFilmDistance( void ) const;
        FLT                     GetExposure( void ) const { return m_nExposure; }
        FLT                     GetMillimetersPerUnit( void ) const { return m_nMillimetersPerUnit; }
        int                     GetSurfaceCount( void ) const { return (int)m_aoSurfaces.size(); }
            /// A line describing the prepared lens, for raythrower.
        std::string             Describe( void ) const;

        void                    ToJson( IASLib::CObjectNode &oNode ) const;
        BOOL                    FromJson( const IASLib::CJsonNode &oNode );

            /// The built-in lenses.
        static const SPrescription *GetPrescriptions( int &nCount );
};

#endif // __LENS_H__
