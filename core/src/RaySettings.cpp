#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "RaySettings.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define SETTINGS_VERSION    ((DWORD)1)
CSettings::CSettings( int nWidth, int nHeight, int nColorDepth, LPCSTR strOutputFile )
{
    m_nImageWidth = nWidth;
    m_nImageHeight = nHeight;
    m_nColorDepth = nColorDepth;
    m_strImageFile = strOutputFile;
    m_bGammaCorrect = TRUE;
    m_nGammaCorrection = 1.0;
    m_bAdaptiveDepth = FALSE;
    m_nAdaptiveDepthThreshold = 0.01;
    m_nMaximumTraceDepth = 15;
    m_bAntialias = FALSE;
    m_bAdaptiveAntialiasing = FALSE;
    m_nAntialiasThreshold = 0.03;
    m_nMaximumAntialiasDepth = 4;
    m_bAtmosphericEffects = FALSE;
    m_bDistanceAttenuation = FALSE;
    m_nDistanceHazeHemidepth = 10.0;
    m_bCaustics = FALSE;
    m_bAdaptiveCaustics = FALSE;
    m_nMaximumCausticDepth = 15;
    m_nCausticThreshold = 0.01;
    m_bSimpleRadiosity = FALSE;
    m_bTraceByColor = FALSE;
    m_bFocalBlur = FALSE;
    m_nDepthOfField = 10.0;
    m_nFocalDisplacement = 2.63;
    m_bAdaptiveFocalBlur = FALSE;
    m_nAdaptiveFocalThreshold = 0.01;
    m_bFractalTurbulence = FALSE;
    m_nFractalSeed = 0.04;
}

CRayStream &operator <<( CRayStream &oAr, CSettings &oSettings )
{
    oAr << SETTINGS_VERSION;
    oAr << oSettings.m_nImageWidth;
    oAr << oSettings.m_nImageHeight;
    oAr << oSettings.m_nColorDepth;
    oAr << oSettings.m_strImageFile;
    oAr << oSettings.m_bGammaCorrect;
    oAr << oSettings.m_nGammaCorrection;
    oAr << oSettings.m_bAdaptiveDepth;
    oAr << oSettings.m_nAdaptiveDepthThreshold;
    oAr << oSettings.m_nMaximumTraceDepth;
    oAr << oSettings.m_bAntialias;
    oAr << oSettings.m_bAdaptiveAntialiasing;
    oAr << oSettings.m_nAntialiasThreshold;
    oAr << oSettings.m_nMaximumAntialiasDepth;
    oAr << oSettings.m_bAtmosphericEffects;
    oAr << oSettings.m_bDistanceAttenuation;
    oAr << oSettings.m_nDistanceHazeHemidepth;
    oAr << oSettings.m_bCaustics;
    oAr << oSettings.m_bAdaptiveCaustics;
    oAr << oSettings.m_nMaximumCausticDepth;
    oAr << oSettings.m_nCausticThreshold;
    oAr << oSettings.m_bSimpleRadiosity;
    oAr << oSettings.m_bTraceByColor;
    oAr << oSettings.m_bFocalBlur;
    oAr << oSettings.m_nDepthOfField;
    oAr << oSettings.m_nFocalDisplacement;
    oAr << oSettings.m_bAdaptiveFocalBlur;
    oAr << oSettings.m_nAdaptiveFocalThreshold;
    oAr << oSettings.m_bFractalTurbulence;
    oAr << oSettings.m_nFractalSeed;

    return oAr;
}

CRayStream &operator >>( CRayStream &oAr, CSettings &oSettings )
{
    DWORD       dwVersion;

    oAr >> dwVersion;
    switch ( dwVersion )
    {
        case SETTINGS_VERSION:
            oAr >> oSettings.m_nImageWidth;
            oAr >> oSettings.m_nImageHeight;
            oAr >> oSettings.m_nColorDepth;
            oAr >> oSettings.m_strImageFile;
            oAr >> oSettings.m_bGammaCorrect;
            oAr >> oSettings.m_nGammaCorrection;
            oAr >> oSettings.m_bAdaptiveDepth;
            oAr >> oSettings.m_nAdaptiveDepthThreshold;
            oAr >> oSettings.m_nMaximumTraceDepth;
            oAr >> oSettings.m_bAntialias;
            oAr >> oSettings.m_bAdaptiveAntialiasing;
            oAr >> oSettings.m_nAntialiasThreshold;
            oAr >> oSettings.m_nMaximumAntialiasDepth;
            oAr >> oSettings.m_bAtmosphericEffects;
            oAr >> oSettings.m_bDistanceAttenuation;
            oAr >> oSettings.m_nDistanceHazeHemidepth;
            oAr >> oSettings.m_bCaustics;
            oAr >> oSettings.m_bAdaptiveCaustics;
            oAr >> oSettings.m_nMaximumCausticDepth;
            oAr >> oSettings.m_nCausticThreshold;
            oAr >> oSettings.m_bSimpleRadiosity;
            oAr >> oSettings.m_bTraceByColor;
            oAr >> oSettings.m_bFocalBlur;
            oAr >> oSettings.m_nDepthOfField;
            oAr >> oSettings.m_nFocalDisplacement;
            oAr >> oSettings.m_bAdaptiveFocalBlur;
            oAr >> oSettings.m_nAdaptiveFocalThreshold;
            oAr >> oSettings.m_bFractalTurbulence;
            oAr >> oSettings.m_nFractalSeed;
            break;

        default:
            break;
    }

    return oAr;
}

#ifdef RAY_GUI
#include "resource.h"
#include "SettingsDialog.h"

void CSettings::Properties( void )
{
    CSettingsDialog     oDlg;

    oDlg.m_nImageWidth = m_nImageWidth;
    oDlg.m_nImageHeight = m_nImageHeight;
    oDlg.m_strFileName = m_strImageFile;

    if ( oDlg.DoModal() == IDOK )
    {
        m_nImageWidth = oDlg.m_nImageWidth;
        m_nImageHeight = oDlg.m_nImageHeight;
        m_strImageFile = oDlg.m_strFileName;
    }
}

#endif
