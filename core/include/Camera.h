/*************************************************************************
*   CCamera                                                              *
*                                                                        *
*       This is the base class for all cameras. This basic camera simply *
* defines a standard 4/3 ratio NTSC type screen.                         *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "Objects.h"
#include "Vector.h"
#include "Matrix.h"
#include "PixelSampler.h"
#include "ColorModel.h"
#include "Lens.h"

#include <memory>

class CCamera : public CObjects
{
    protected:
        CVector     m_oPosition;
        CVector     m_oPointAt;
        CVector     m_oUp;
        FLT         m_nVerticalAngle;
        FLT         m_nHorizontalAngle;
        FLT         m_nAspectRatio;
        FLT         m_nXRotate;
        FLT         m_nYRotate;

            // The camera's viewing basis, worked out by CalcRotate(): the
            // direction it looks, and the directions of the image's right
            // and top edges, all unit vectors.
        CVector     m_oForward;
        CVector     m_oRight;
        CVector     m_oTop;

            // How the camera samples each pixel; NULL means the default,
            // which GetSampler makes when it is first needed (cameras read
            // from a stream don't go through the other constructors). Not
            // saved in scene files.
        std::unique_ptr<CPixelSampler>  m_pSampler;
            // How the camera turns pixel colors into bytes; NULL means the
            // default, as for the sampler.
        std::unique_ptr<CColorModel>    m_pColorModel;
            // A real lens, or NULL for a pinhole camera.
        std::unique_ptr<CLens>          m_pLens;
        BOOL                            m_bAimOnly = FALSE;     // see SetAimOnly
            // The brightest any one sample's red, green or blue may be before
            // the sampler averages it in; 0 doesn't clamp.
        FLT                             m_nClamp = 0.0;
            // How light splits into colors in dispersive glass and a
            // chromatic lens: into red, green and blue, or into single
            // wavelengths (see Spectrum.h).
        BOOL                            m_bSpectral = FALSE;
        BOOL                            m_bPanorama = FALSE;    // equirectangular 360 x 180 projection
        std::string                     m_strLensError;     // why the lens couldn't be prepared
    public:
            // A Reseau plate: a glass plate pressed against the film,
            // engraved with a grid of opaque crosses that print on every
            // frame (the Hasselblad Data Camera's 5 by 5, 10 mm apart). The
            // crosses block the light that would reach the film under them;
            // the plate's glass is part of the lens's prescription.
        struct SReseau
        {
            BOOL    bOn = FALSE;
            int     nCount = 5;             // crosses on a side
            FLT     nSpacing = 10.0;        // mm between crosses
            FLT     nSize = 1.2;            // mm across each cross
            FLT     nLineWidth = 0.05;      // mm
        };

            // A camera body: a lens, a format, a film and maybe a Reseau
            // plate, for "body" in a scene file.
        struct SBody
        {
            const char *strName;
            const char *strDescription;
            const char *strLens;            // a built-in lens prescription
            FLT         nFormat;            // the image's width on the film, mm
            FLT         nFStop;             // the film's default exposure stop
            BOOL        bReseau;
        };
        static const SBody *GetBodies( int &nCount );

            /// Whether a point on the film (mm from the frame's center) lies
            /// under one of the Reseau plate's crosses.
        BOOL        OnReseau( FLT nX, FLT nY ) const;
        const SReseau &GetReseau( void ) const { return m_oReseau; }
        void        SetReseau( const SReseau &oReseau ) { m_oReseau = oReseau; }
    protected:
        SReseau                         m_oReseau;
    public:
                    CCamera( void );
                    CCamera( CVector oPosition, CVector oPointAt, FLT nVerticalAngle, FLT nHorizontalAngle, FLT nAspectRatio = 0.0, CVector *poUp = NULL );
                    CCamera( const CCamera &oSource );
        virtual    ~CCamera( void );

        virtual BOOL    IsCamera( void ) { return TRUE; }

        CCamera    &operator =( const CCamera &oSource );

        FLT         GetHorizontalAngle( void ) { return m_nHorizontalAngle; }
        FLT         GetVerticalAngle( void ) { return m_nVerticalAngle; }
        CVector     GetPosition( void ) { return m_oPosition; }
        CVector     PointAt( void ) { return m_oPointAt; }

        DYNAMIC_OBJECTS( CCamera );


        void        CalcRotate( void );

            // The direction of the ray through a point on the image. nScreenX
            // runs from -1 at the left edge to 1 at the right, nScreenY from -1
            // at the bottom to 1 at the top, and nAspect is the image's width
            // over its height. The vertical angle sets the field of view; the
            // horizontal one follows from the image's shape, so pixels are square.
        CVector     RayDirection( FLT nScreenX, FLT nScreenY, FLT nAspect );

            // The sampler that makes the camera's primary rays and turns their
            // colors into pixels. A camera starts with a CCenterSampler.
            // SetSampler takes ownership; NULL puts the default back. Copying
            // a camera copies its sampler.
        CPixelSampler  &GetSampler( void );
        void        SetSampler( CPixelSampler *pSampler );

            // The color model that turns the sampler's pixel colors into the
            // image's bytes. A camera starts with a CClipColorModel, which
            // writes colors as the 1997 code did. SetColorModel takes
            // ownership; NULL puts the default back. Copying a camera copies
            // its color model.
        CColorModel    &GetColorModel( void );
        void        SetColorModel( CColorModel *pColorModel );

            // The camera's lens: NULL for a pinhole camera (the default),
            // which sees with the vertical angle; with a lens the field of
            // view comes from its focal length and the sensor. SetLens takes
            // ownership and prepares the lens to focus at its focus distance,
            // or at the look-at point; it returns FALSE, with the reason in
            // GetLensError, if the lens can't be used.
        CLens      *GetLens( void ) { return m_pLens.get(); }
            // While set, primary rays are only aimed, as by a pinhole, not
            // traced through the lens: for a ray wanted only for where it
            // falls on the image (a chromatic lens's sampler ray, which is
            // made again for each channel).
        void        SetAimOnly( BOOL bAimOnly ) { m_bAimOnly = bAimOnly; }
        BOOL        SetLens( CLens *pLens );
        const char *GetLensError( void ) const { return m_strLensError.c_str(); }

            // The primary ray for a point on the image (as for RayDirection;
            // nImageX and nImageY, the point in pixels, pick where a ray
            // crosses the lens). FALSE if the lens blocks it. pnWeight, if
            // given, gets the factor the ray's color is multiplied by.
        BOOL        PrimaryRay( FLT nScreenX, FLT nScreenY, FLT nAspect, FLT nImageX, FLT nImageY,
                                CVector &oOrigin, CVector &oDirection, int nChannel = 0, FLT *pnWeight = NULL );
            // How many channels each sample is traced in: 3 for a lens with
            // chromatic aberration (red, green and blue apart), else 1.
        int         GetChannels( void ) const { return ( m_pLens && m_pLens->IsChromatic() ) ? 3 : 1; }
            // What every pixel is scaled by: 1 for a pinhole.
        FLT         GetExposure( void ) const { return m_pLens ? m_pLens->GetExposure() : 1.0; }
            // The sample clamp ("clamp" in JSON): 0 for none. A rare ray that
            // finds a light through rough glass, a rough mirror or an area
            // light can be far brighter than its pixel's others and show as
            // a lone bright speck ("firefly"); clamping each sample at, say,
            // 2 keeps it to its share, at the cost of a little of the
            // brightest highlights.
        FLT         GetClamp( void ) const { return m_nClamp; }
        void        SetClamp( FLT nClamp ) { m_nClamp = nClamp; }
            // Whether dispersion is traced wavelength by wavelength
            // ("dispersion": "spectral") rather than in red, green and blue.
        BOOL        IsSpectral( void ) const { return m_bSpectral; }
        void        SetSpectral( BOOL bSpectral ) { m_bSpectral = bSpectral; }
            /// An equirectangular panorama: the image's width is all the way
            /// round (longitude, the look-at direction in the middle) and its
            /// height from straight up to straight down, in the layout an
            /// `environment` image uses, so a rendered panorama can be read
            /// back as one.
        BOOL        IsPanorama( void ) const { return m_bPanorama; }
        void        SetPanorama( BOOL bPanorama ) { m_bPanorama = bPanorama; }
            // A sample's color with each channel clamped (unchanged if the
            // camera doesn't clamp).
        CColor      ClampSample( const CColor &oColor ) const;
        FLT         GetXRotate( void ) { return m_nXRotate; }
        FLT         GetYRotate( void ) { return m_nYRotate; }

};

#endif
