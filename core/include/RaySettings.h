#ifndef __RAYSETTINGS_H__
#define __RAYSETTINGS_H__

/*************************************************************************
*   CSettings                                                            *
*                                                                        *
*       To-Be-Completed, this class will store all of the persistantly   *
* held settings of the ray-tracer. Right now, it is mostly a place-      *
* holder for use in the CScene class.                                    *
*                                                                        *
* Copyright (c) 1997, Jeffrey R. Naujok                                  *
*************************************************************************/

#include "Globals.h"

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

class CSettings
{
    public:
                        CSettings( int nWidth = 640, int nHeight = 480, int nColorDepth = 8, LPCSTR strOutputFile = "ray.out" );
        virtual        ~CSettings( void ) {}

            // The settings as JSON for a scene file. The ones the tracer uses
            // are fields of oNode; the 1997 placeholders it never implemented
            // go in "reserved1997", and only where they differ from their
            // defaults. Every field is optional when reading.
        void            ToJson( IASLib::CObjectNode &oNode );
        BOOL            FromJson( const IASLib::CJsonNode &oNode );

    protected:
            // Image information
        WORD            m_nImageWidth;
        WORD            m_nImageHeight;
        WORD            m_nColorDepth;
        IASLib::CString         m_strImageFile;

            // Use of Gamma correction
        BOOL            m_bGammaCorrect;
        FLT             m_nGammaCorrection;

            // Depth (re-casts of rays) allowed
        BOOL            m_bAdaptiveDepth;
        FLT             m_nAdaptiveDepthThreshold;
        WORD            m_nMaximumTraceDepth;

            // Antialiasing
        BOOL            m_bAntialias;
        BOOL            m_bAdaptiveAntialiasing;
        FLT             m_nAntialiasThreshold;
        WORD            m_nMaximumAntialiasDepth;

            // Atmospheric effects turned on.
        BOOL            m_bAtmosphericEffects;

            // Distance attenuation (i.e. Distance haze )
        BOOL            m_bDistanceAttenuation;
        FLT             m_nDistanceHazeHemidepth;

            // Simple Caustics turned on.
        BOOL            m_bCaustics;

            // Adaptive Caustics turned on. Based on Photon mapping the scene.
        BOOL            m_bAdaptiveCaustics;
        WORD            m_nMaximumCausticDepth;
        FLT             m_nCausticThreshold;

            // Simple Radiosity method active.
        BOOL            m_bSimpleRadiosity;

            // Trace each color (Red, Blue and Green) with it's own ray. This
            // is only effective for refractive items with a different IOR for
            // each color.
        BOOL            m_bTraceByColor;

            // Use focal blurring. Need to specify the depth of field where
            // focus is perfect. Automatically shuts off anti-aliasing. This
            // default level casts five rays per pixel, each offset by the
            // focal displacement number of pixels. If the focal displacement
            // is less than 0.50, then this method simply becomes a form of
            // simple anti-aliasing.
        BOOL            m_bFocalBlur;
        FLT             m_nDepthOfField;
        FLT             m_nFocalDisplacement;

            // Adaptive focal blur, similar to adaptive antialiasing, casts
            // multiple rays per pixel to reach a happy average, but these rays
            // are all cast starting much wider then the given pixel, and then
            // they all are pointing at a single point at the focal "depth of field"
            // distance. From that point, they diverge again. The average of all
            // those rays forms the basis for the color. If any pixel differs
            // signifigantly from it's neighbors (by more than the adaptive focal
            // blur threshold, then the square formed by this item and it's neighbors
            // is subdivided until the difference becomes less than the threshold.
        BOOL            m_bAdaptiveFocalBlur;
        FLT             m_nAdaptiveFocalThreshold;

            // Use fractal turbulence uses a fractal noise generator rather than
            // a 3-d static noise function.
        BOOL            m_bFractalTurbulence;
        FLT             m_nFractalSeed;

    public:
        void            SetImageWidth( int nWidth ) 
                        { 
                            m_nImageWidth = nWidth; 
                        }

        void            SetImageHeight( int nHeight ) 
                        { 
                            m_nImageHeight = nHeight; 
                        }

        int             GetImageWidth( void ) 
                        { 
                            return m_nImageWidth; 
                        }

        int             GetImageHeight( void ) 
                        { 
                            return m_nImageHeight; 
                        }

        void            SetColorDepth( int nDepth ) 
                        { 
                            m_nColorDepth = nDepth; 
                        }
        int             GetColorDepth( void ) 
                        { 
                            return m_nColorDepth; 
                        }

        void            SetImageFileName( LPCSTR strImageFile )
                        {
                            m_strImageFile = strImageFile;
                        }
        LPCSTR          GetImageFileName( void )
                        { 
                            return m_strImageFile; 
                        }

        void            SetGammaCorrect( FLT nGammaLevel )
                        {
                            m_bGammaCorrect = (nGammaLevel == 1.0);
                            m_nGammaCorrection = nGammaLevel;
                        }

        BOOL            IsGammaCorrect( void ) 
                        { 
                            return m_bGammaCorrect; 
                        }

        FLT             GetGammaCorrection( void ) 
                        {
                            return m_nGammaCorrection; 
                        }

        void            SetAdaptiveDepth( BOOL bOn ) 
                        { 
                            m_bAdaptiveDepth = bOn; 
                        }

        BOOL            IsAdaptiveDepth( void ) 
                        { 
                            return m_bAdaptiveDepth; 
                        }

        void            SetDepthThreshold( FLT nThreshold ) 
                        { 
                            m_nAdaptiveDepthThreshold = nThreshold; 
                        }

        FLT             GetDepthThreshold( void ) 
                        { 
                            return m_nAdaptiveDepthThreshold; 
                        }
        void            SetMaximumDepth( int nDepth ) 
                        { 
                            m_nMaximumTraceDepth = nDepth; 
                        }
        int             GetMaximumDepth( void ) 
                        { 
                            return m_nMaximumTraceDepth; 
                        }

        void            SetAntialiasing( BOOL bOn ) 
                        { 
                            m_bAntialias = bOn; 
                        }
        BOOL            IsAntialising( void ) 
                        { 
                            return m_bAntialias; 
                        }
        void            SetAdaptiveAntialiasing( BOOL bOn ) 
                        { 
                            m_bAdaptiveAntialiasing = bOn; 
                        }
        BOOL            IsAdaptiveAntialiasing( void ) 
                        { 
                            return m_bAdaptiveAntialiasing; 
                        }
        void            SetAntialiasThreshold( FLT nThreshold ) 
                        { 
                            m_nAntialiasThreshold = nThreshold; 
                        }
        FLT             GetAntialiasThreshold( void ) 
                        { 
                            return m_nAntialiasThreshold; 
                        }
        void            SetMaximumAntialiasDepth( int nDepth ) 
                        { 
                            m_nMaximumAntialiasDepth = nDepth; 
                        }
        int             GetMaximumAntialiasDepth( void ) 
                        { 
                            return m_nMaximumAntialiasDepth; 
                        }

        void            SetAtmosphericEffects( BOOL bOn ) 
                        { 
                            m_bAtmosphericEffects = bOn; 
                        }
        BOOL            IsAtmposphericEffects( void ) 
                        { 
                            return m_bAtmosphericEffects; 
                        }

        void            SetDistanceAttenuation( BOOL bOn ) { m_bDistanceAttenuation = bOn; }
        BOOL            IsDistanceAttenuation( void ) { return m_bDistanceAttenuation; }
        void            SetDistanceHazeDepth( FLT nDepth )
                        {
                            m_nDistanceHazeHemidepth = ABS(nDepth);
                            if ( m_nDistanceHazeHemidepth > 0.0 )
                                m_bDistanceAttenuation = TRUE;
                        }
        FLT             GetDistanceHazeDepth( void ) { return m_nDistanceHazeHemidepth; }

        void            SetCaustics( BOOL bOn ) { m_bCaustics = bOn; }
        BOOL            IsCaustics( void ) { return m_bCaustics; }

        void            SetAdaptiveCaustics( BOOL bOn ) 
                        { 
                            if ( bOn ) 
                                m_bCaustics = bOn; 
                            m_bAdaptiveCaustics = bOn; 
                        }
        BOOL            IsAdaptiveCaustics( void ) { return m_bAdaptiveCaustics; }
        void            SetCausticDepth( int nDepth ) { m_nMaximumCausticDepth = nDepth; }
        int             GetCausticDepth( void ) { return m_nMaximumCausticDepth; }
        void            SetCausticThreshold( FLT nThresh ) { m_nCausticThreshold = nThresh; }
        FLT             GetCausticThreshold( void ) { return m_nCausticThreshold; }

        void            SetRadiosity( BOOL bOn ) { m_bSimpleRadiosity = bOn; }
        BOOL            IsRadiosity( void ) { return m_bSimpleRadiosity; }

        void            SetTraceByColor( BOOL bOn ) { m_bTraceByColor = bOn; }
        BOOL            IsTraceByColor( void ) { return m_bTraceByColor; }

        void            SetFocalBlur( BOOL bOn ) { m_bFocalBlur = bOn; }
        BOOL            IsFocalBlur( void ) { return m_bFocalBlur; }
        void            SetFocalDepthOfField( FLT nDepth ) { m_nDepthOfField = nDepth; }
        FLT             GetFocalDepthOfField( void ) { return m_nDepthOfField; }
        void            SetFocalDisplacement( FLT nDisplace ) { m_nFocalDisplacement = nDisplace; }
        FLT             GetFocalDisplacement( void ) { return m_nFocalDisplacement; }

        void            SetAdaptiveFocalBlur( BOOL bOn )
                        {
                            if ( bOn )
                                m_bFocalBlur = bOn;
                            m_bAdaptiveFocalBlur = bOn;
                        }
        BOOL            IsAdaptiveFocalBlur( void ) { return m_bAdaptiveFocalBlur; }
        void            SetFocalThreshold( FLT nDepth ) { m_nAdaptiveFocalThreshold = nDepth; }
        FLT             GetFocalThreshold( void ) { return m_nAdaptiveFocalThreshold; }

        void            SetFractalTurbulence( BOOL bOn ) { m_bFractalTurbulence = bOn; }
        BOOL            IsFractalTurbulence( void ) { return m_bFractalTurbulence; }
        void            SetFractalSeed( FLT nSeed ) { m_nFractalSeed = nSeed; }
        FLT             GetFractalSeed( void ) { return m_nFractalSeed; }


#ifdef RAY_GUI
        void            Properties( void );
#endif
};

#endif

