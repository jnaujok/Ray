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
*       glass       or a crystal or glass by name, traced with its own   *
*                   Sellmeier formula: fused silica or calcium fluoride  *
*                   (they disperse too little for a Cauchy fit to hold)  *
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
#include <memory>
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
            int                 nGlass;         // a Sellmeier glass (GLASS_*), or 0 for nIor and nAbbe
        };

            // Glasses traced with their Sellmeier formulas.
        enum EGlass { GLASS_NONE = 0, GLASS_FUSED_SILICA = 1, GLASS_CALCIUM_FLUORIDE = 2 };

            /// A Sellmeier glass's index at a wavelength in micrometers.
        static FLT              SellmeierIndex( int nGlass, FLT nMicrometers );
            /// A glass's name in scene files ("fused-silica"), or NULL.
        static const char      *GlassName( int nGlass );
            /// The glass of that name, or GLASS_NONE.
        static int              GlassByName( const std::string &strName );

            // An exit pupil: an ellipse on the rear element (in mm, for a
            // point of the sensor on the +x axis) around everywhere rays get
            // through.
        struct SPupil
        {
            FLT                 nCenterX;
            FLT                 nCenterY;
            FLT                 nHalfX;             // the ellipse's half widths
            FLT                 nHalfY;
            BOOL                bOpen;              // FALSE: no light gets through at all
        };

            // The channel a ray is traced in: every channel at the d line, or
            // one of red, green and blue at its own wavelength, or (with
            // spectral dispersion) a single wavelength: CHANNEL_SPECTRAL plus
            // the wavelength in hundredths of a nanometer.
        enum EChannel { CHANNEL_ALL = 0, CHANNEL_RED = 1, CHANNEL_GREEN = 2, CHANNEL_BLUE = 3, CHANNEL_SPECTRAL = 0x10000 };

            /// The channel for a single wavelength, in nanometers.
        static int              WavelengthChannel( FLT nNanometers );
            /// Whether a channel is a single wavelength.
        static BOOL             IsSpectral( int nChannel ) { return ( nChannel >= CHANNEL_SPECTRAL ) ? TRUE : FALSE; }
            /// A single-wavelength channel's wavelength, in nanometers.
        static FLT              ChannelWavelength( int nChannel ) { return ( nChannel - CHANNEL_SPECTRAL ) / 100.0; }
            /// The index at a wavelength (in micrometers) of a glass with
            /// index nIor at the d line and Abbe number nAbbe, from the Cauchy
            /// curve through both.
        static FLT              IndexAt( FLT nIor, FLT nAbbe, FLT nMicrometers );

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
            // What follows each surface (from -1, the air in front) in each
            // channel, worked out once by Prepare: IorAt is in every step of
            // every ray through the lens.
        std::vector<FLT>        m_anIorTable[ 4 ];
        FLT                     m_nFocalLength;
        FLT                     m_nWideOpen;        // the smallest f-number
        FLT                     m_nStopRadius;      // the stop's opening at the f-number
        FLT                     m_nSensorZ;
        FLT                     m_nFocusedAt;       // the focus distance used, in scene units
        FLT                     m_nExposure;
        BOOL                    m_bPrepared;

            // The exit pupil: for sensor points in each ring out from the
            // center, an ellipse on the rear element (in mm, for a point on
            // the +x axis) around everywhere rays get through. Rays are aimed
            // only inside it.
            // The exit pupil for each ring of the sensor, and the share of
            // rays aimed at the rear element that get through at each ring's
            // edges in each channel traced. Prepare works them out (on every
            // core) unless the scene file brought them for the same lens
            // setup; they go into the scene, so tasks never work them out.
        struct STables
        {
            std::vector<SPupil> aoRings;
            std::vector<FLT>    anPass[ 4 ];        // by channel; empty if not traced
        };
        std::shared_ptr<const STables> m_pTables;
        std::string             m_strTablesKey;     // what m_pTables were worked out for
        std::shared_ptr<const STables> m_pLoadedTables; // from the scene file, until Prepare checks them
        std::string             m_strLoadedKey;
        FLT                     m_nPupilReach;      // the sensor radius the rings cover, in mm

            /// What the tables depend on (the surfaces, stop, sensor and
            /// channels), as a short fingerprint.
        std::string             TablesKey( void ) const;
            /// Works out the tables on every core.
        void                    BuildTables( void );
            /// Works out one ring's exit pupil.
        void                    FindPupil( int nRing, SPupil &oPupil ) const;
            /// The share of rays aimed anywhere on the rear element from a
            /// point nKnot rings out on the sensor that get through.
        FLT                     FindPass( int nKnot, int nChannel ) const;

            // The surface rays from the sensor are aimed at: the last
            // curved one or the stop, so a flat plate in front of the film
            // (a Reseau plate) is traced through but not aimed at.
        size_t                  AimIndex( void ) const
        {
            for ( size_t nIndex = m_aoSurfaces.size(); nIndex > 0; nIndex-- )
            {
                if ( m_aoSurfaces[ nIndex - 1 ].nRadius != 0.0 || m_aoSurfaces[ nIndex - 1 ].bStop )
                    return nIndex - 1;
            }
            return m_aoSurfaces.empty() ? 0 : m_aoSurfaces.size() - 1;
        }

            // Traces a ray through the surfaces, forward (toward the
            // sensor) or backward. The ray is changed in place; FALSE if it
            // is blocked or totally internally reflected.
        BOOL                    Trace( FLT anOrigin[ 3 ], FLT anDirection[ 3 ], BOOL bForward, FLT nStopRadius,
                                       int nChannel = CHANNEL_ALL, std::vector<FLT> *panPath = NULL ) const;
            // The index of what follows surface nIndex (or air, for -1) in a channel.
        FLT                     IorAt( int nIndex, int nChannel ) const;
        FLT                     WorkOutIor( int nIndex, int nChannel ) const;
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
        void                    SetSensorWidth( FLT nMillimeters ) { m_nSensorWidth = nMillimeters; m_bPrepared = FALSE; }
        FLT                     GetSensorWidth( void ) const { return m_nSensorWidth; }

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
            /// element: an even spread of them covers, evenly, the part of it
            /// light gets through to that point of the sensor (the exit
            /// pupil). pnWeight, if given, gets the factor the ray's color
            /// counts for: the share of the rear element that part is, so the
            /// image is as bright as sampling all of it. FALSE if the lens
            /// blocks the ray.
        BOOL                    RayFor( FLT nScreenX, FLT nScreenY, FLT nAspect, FLT nLensU, FLT nLensV,
                                        FLT anOrigin[ 3 ], FLT anDirection[ 3 ], int nChannel = CHANNEL_ALL,
                                        FLT *pnWeight = NULL ) const;
            /// Whether the channels are traced apart: some glass has an Abbe
            /// number, and chromatic aberration isn't turned off.
        BOOL                    IsChromatic( void ) const;
            /// The focal length for a channel's wavelength (after Prepare).
        FLT                     FocalLengthIn( int nChannel ) const;
            /// The index, in a channel, of a glass with index nIor at the d
            /// line and Abbe number nAbbe (0: it doesn't disperse), from a
            /// Cauchy curve through both: red at the C line, green at the d
            /// line (nIor itself), blue at the F line, or a single-wavelength
            /// channel at its own. Dispersive transparent materials use it too.
        static FLT              DispersedIndex( FLT nIor, FLT nAbbe, int nChannel );
        int                     GetSeed( void ) const { return m_nSeed; }

        FLT                     GetFocalLength( void ) const { return m_nFocalLength; }
        FLT                     GetWideOpen( void ) const { return m_nWideOpen; }
        FLT                     GetFNumber( void ) const { return ( m_nFNumber > 0.0 ) ? m_nFNumber : m_nWideOpen; }
        FLT                     GetFocusedAt( void ) const { return m_nFocusedAt; }
            /// From the rear surface to the sensor, in mm.
        FLT                     GetFilmDistance( void ) const;
        FLT                     GetExposure( void ) const { return m_nExposure; }
            /// Whether the exit pupil tables are worked out (after Prepare).
        BOOL                    HasTables( void ) const { return m_pTables ? TRUE : FALSE; }
        FLT                     GetMillimetersPerUnit( void ) const { return m_nMillimetersPerUnit; }
        int                     GetSurfaceCount( void ) const { return (int)m_aoSurfaces.size(); }
            /// A line describing the prepared lens, for raythrower.
        std::string             Describe( void ) const;

        void                    ToJson( IASLib::CObjectNode &oNode ) const;
            /// Reads the lens. With bHasLens (a camera body gave one), the
            /// prescription and surfaces may be left out to keep it.
        BOOL                    FromJson( const IASLib::CJsonNode &oNode, BOOL bHasLens = FALSE );

            /// The built-in lenses.
        static const SPrescription *GetPrescriptions( int &nCount );

            /// The surfaces, as given or from the prescription.
        const std::vector<SSurface> &GetSurfaces( void ) const { return m_aoSurfaces; }
            /// Where surface nIndex's vertex is on the axis, in mm from the
            /// front vertex (after Prepare).
        FLT                     GetVertexZ( int nIndex ) const { return m_anVertexZ[ (size_t)nIndex ]; }
            /// Where the sensor is on the axis, in mm (after Prepare).
        FLT                     GetSensorZ( void ) const { return m_nSensorZ; }
            /// The stop's opening at the f-number, in mm (after Prepare).
        FLT                     GetStopRadius( void ) const { return m_nStopRadius; }
            /// The index of what follows surface nIndex (-1: the air in
            /// front) at the d line, and its Abbe number (0 for air), for
            /// building the lens's glass (after Prepare).
        void                    GlassAfter( int nIndex, FLT &nIndexD, FLT &nAbbe ) const;
            /// Traces a ray from in front of the lens to the sensor plane
            /// (after Prepare), in the lens frame and in mm, at the f-number,
            /// in a channel (CHANNEL_ALL, a color, or a spectral one). The
            /// point where it meets each surface, then the sensor plane, go
            /// into anPoints as x, y, z. FALSE if the lens blocks it.
        BOOL                    TracePath( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], int nChannel,
                                           std::vector<FLT> &anPoints ) const;
};

#endif // __LENS_H__
