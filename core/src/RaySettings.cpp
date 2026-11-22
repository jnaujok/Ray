    #include "Globals.h"

#include "RaySettings.h"
#include "SceneJson.h"


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
    m_nMaximumTraceDepth = 16;
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
    m_nIndirectBounces = 0;
    m_nIndirectSamples = 4;
    m_nIndirectDistance = 10.0;
    m_nIndirectStrength = 1.0;
    m_nIndirectMethod = INDIRECT_OBJECTS;
    m_nIndirectObjects = 8;
}


    // A WORD setting from minimum through 65535.
static BOOL GetWord( const IASLib::CJsonNode &oNode, const char *strKey, WORD &nValue, int nMinimum )
{
    int nRead = nValue;

    if ( ! RayJson::GetInt( oNode, strKey, nRead, nMinimum, 65535, FALSE ) )
        return FALSE;
    nValue = (WORD)nRead;
    return TRUE;
}

void CSettings::ToJson( IASLib::CObjectNode &oNode )
{
    CSettings oDefault;

    oNode.put( "imageWidth", (int)m_nImageWidth );
    oNode.put( "imageHeight", (int)m_nImageHeight );
    oNode.put( "imageFile", (const char *)m_strImageFile );
    oNode.put( "colorDepth", (int)m_nColorDepth );
    oNode.put( "maximumDepth", (int)m_nMaximumTraceDepth );
    RayJson::PutNumber( oNode, "depthThreshold", m_nAdaptiveDepthThreshold );
    if ( m_nIndirectBounces > 0 )
    {
        IASLib::CObjectNode *pIndirect = oNode.putObject( "indirect" );

        pIndirect->put( "method", ( m_nIndirectMethod == INDIRECT_HEMISPHERE ) ? "hemisphere" : "objects" );
        pIndirect->put( "bounces", m_nIndirectBounces );
        pIndirect->put( "samples", m_nIndirectSamples );
        if ( m_nIndirectMethod == INDIRECT_OBJECTS )
            pIndirect->put( "objects", m_nIndirectObjects );
        RayJson::PutNumber( *pIndirect, "distance", m_nIndirectDistance );
        RayJson::PutNumber( *pIndirect, "strength", m_nIndirectStrength );
    }

    IASLib::CObjectNode *pReserved = new IASLib::CObjectNode;
    if ( m_bGammaCorrect != oDefault.m_bGammaCorrect ) RayJson::PutBool( *pReserved, "gammaCorrect", m_bGammaCorrect );
    if ( RayJson::Rounded( m_nGammaCorrection ) != RayJson::Rounded( oDefault.m_nGammaCorrection ) ) RayJson::PutNumber( *pReserved, "gammaCorrection", m_nGammaCorrection );
    if ( m_bAdaptiveDepth != oDefault.m_bAdaptiveDepth ) RayJson::PutBool( *pReserved, "adaptiveDepth", m_bAdaptiveDepth );
    if ( m_bAntialias != oDefault.m_bAntialias ) RayJson::PutBool( *pReserved, "antialias", m_bAntialias );
    if ( m_bAdaptiveAntialiasing != oDefault.m_bAdaptiveAntialiasing ) RayJson::PutBool( *pReserved, "adaptiveAntialiasing", m_bAdaptiveAntialiasing );
    if ( RayJson::Rounded( m_nAntialiasThreshold ) != RayJson::Rounded( oDefault.m_nAntialiasThreshold ) ) RayJson::PutNumber( *pReserved, "antialiasThreshold", m_nAntialiasThreshold );
    if ( m_nMaximumAntialiasDepth != oDefault.m_nMaximumAntialiasDepth ) pReserved->put( "maximumAntialiasDepth", (int)m_nMaximumAntialiasDepth );
    if ( m_bAtmosphericEffects != oDefault.m_bAtmosphericEffects ) RayJson::PutBool( *pReserved, "atmosphericEffects", m_bAtmosphericEffects );
    if ( m_bDistanceAttenuation != oDefault.m_bDistanceAttenuation ) RayJson::PutBool( *pReserved, "distanceAttenuation", m_bDistanceAttenuation );
    if ( RayJson::Rounded( m_nDistanceHazeHemidepth ) != RayJson::Rounded( oDefault.m_nDistanceHazeHemidepth ) ) RayJson::PutNumber( *pReserved, "distanceHazeHemidepth", m_nDistanceHazeHemidepth );
    if ( m_bCaustics != oDefault.m_bCaustics ) RayJson::PutBool( *pReserved, "caustics", m_bCaustics );
    if ( m_bAdaptiveCaustics != oDefault.m_bAdaptiveCaustics ) RayJson::PutBool( *pReserved, "adaptiveCaustics", m_bAdaptiveCaustics );
    if ( m_nMaximumCausticDepth != oDefault.m_nMaximumCausticDepth ) pReserved->put( "maximumCausticDepth", (int)m_nMaximumCausticDepth );
    if ( RayJson::Rounded( m_nCausticThreshold ) != RayJson::Rounded( oDefault.m_nCausticThreshold ) ) RayJson::PutNumber( *pReserved, "causticThreshold", m_nCausticThreshold );
    if ( m_bSimpleRadiosity != oDefault.m_bSimpleRadiosity ) RayJson::PutBool( *pReserved, "simpleRadiosity", m_bSimpleRadiosity );
    if ( m_bTraceByColor != oDefault.m_bTraceByColor ) RayJson::PutBool( *pReserved, "traceByColor", m_bTraceByColor );
    if ( m_bFocalBlur != oDefault.m_bFocalBlur ) RayJson::PutBool( *pReserved, "focalBlur", m_bFocalBlur );
    if ( RayJson::Rounded( m_nDepthOfField ) != RayJson::Rounded( oDefault.m_nDepthOfField ) ) RayJson::PutNumber( *pReserved, "depthOfField", m_nDepthOfField );
    if ( RayJson::Rounded( m_nFocalDisplacement ) != RayJson::Rounded( oDefault.m_nFocalDisplacement ) ) RayJson::PutNumber( *pReserved, "focalDisplacement", m_nFocalDisplacement );
    if ( m_bAdaptiveFocalBlur != oDefault.m_bAdaptiveFocalBlur ) RayJson::PutBool( *pReserved, "adaptiveFocalBlur", m_bAdaptiveFocalBlur );
    if ( RayJson::Rounded( m_nAdaptiveFocalThreshold ) != RayJson::Rounded( oDefault.m_nAdaptiveFocalThreshold ) ) RayJson::PutNumber( *pReserved, "adaptiveFocalThreshold", m_nAdaptiveFocalThreshold );
    if ( m_bFractalTurbulence != oDefault.m_bFractalTurbulence ) RayJson::PutBool( *pReserved, "fractalTurbulence", m_bFractalTurbulence );
    if ( RayJson::Rounded( m_nFractalSeed ) != RayJson::Rounded( oDefault.m_nFractalSeed ) ) RayJson::PutNumber( *pReserved, "fractalSeed", m_nFractalSeed );
    if ( pReserved->size() )
        oNode.set( "reserved1997", pReserved );
    else
        delete pReserved;
}

BOOL CSettings::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "imageWidth", "imageHeight", "imageFile", "colorDepth", "maximumDepth",
                                              "depthThreshold", "indirect", "reserved1997", NULL };
    static const char *const astrIndirect[] = { "method", "bounces", "samples", "objects", "distance", "strength", NULL };
    static const char *const astrReserved[] = { "gammaCorrect", "gammaCorrection", "adaptiveDepth", "antialias", "adaptiveAntialiasing", "antialiasThreshold", "maximumAntialiasDepth", "atmosphericEffects", "distanceAttenuation", "distanceHazeHemidepth", "caustics", "adaptiveCaustics", "maximumCausticDepth", "causticThreshold", "simpleRadiosity", "traceByColor", "focalBlur", "depthOfField", "focalDisplacement", "adaptiveFocalBlur", "adaptiveFocalThreshold", "fractalTurbulence", "fractalSeed", NULL };
    std::string strImageFile = (const char *)m_strImageFile;

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! GetWord( oNode, "imageWidth", m_nImageWidth, 1 ) ||
         ! GetWord( oNode, "imageHeight", m_nImageHeight, 1 ) ||
         ! RayJson::GetString( oNode, "imageFile", strImageFile, FALSE ) ||
         ! GetWord( oNode, "colorDepth", m_nColorDepth, 1 ) ||
         ! GetWord( oNode, "maximumDepth", m_nMaximumTraceDepth, 0 ) ||
         ! RayJson::GetNumber( oNode, "depthThreshold", m_nAdaptiveDepthThreshold, FALSE ) )
        return FALSE;
    m_strImageFile = strImageFile.c_str();

    const IASLib::CJsonNode *pIndirect = oNode.get( "indirect" );

    if ( pIndirect )
    {
        RayJson::CWhere oWhere( "indirect" );

        std::string strMethod = "objects";

        if ( ! pIndirect->isObject() )
            return RayJson::Fail( "must be an object" );
        m_nIndirectObjects = 8;
        if ( ! RayJson::OnlyFields( *pIndirect, astrIndirect ) ||
             ! RayJson::GetString( *pIndirect, "method", strMethod, FALSE ) ||
             ! RayJson::GetInt( *pIndirect, "objects", m_nIndirectObjects, 1, MAXIMUM_INDIRECT_OBJECTS, FALSE ) ||
             ! RayJson::GetInt( *pIndirect, "bounces", m_nIndirectBounces, 0, MAXIMUM_INDIRECT_BOUNCES, TRUE ) ||
             ! RayJson::GetInt( *pIndirect, "samples", m_nIndirectSamples, 1, MAXIMUM_INDIRECT_SAMPLES, FALSE ) ||
             ! RayJson::GetNumber( *pIndirect, "distance", m_nIndirectDistance, FALSE ) ||
             ! RayJson::GetNumber( *pIndirect, "strength", m_nIndirectStrength, FALSE ) )
            return FALSE;
        if ( strMethod == "objects" )
            m_nIndirectMethod = INDIRECT_OBJECTS;
        else if ( strMethod == "hemisphere" )
            m_nIndirectMethod = INDIRECT_HEMISPHERE;
        else
            return RayJson::Fail( "unknown method \"" + strMethod + "\" (objects or hemisphere)" );
        if ( m_nIndirectMethod == INDIRECT_HEMISPHERE && pIndirect->get( "objects" ) )
            return RayJson::Fail( "objects is only for the objects method" );
        if ( ! ( m_nIndirectDistance > 0.0 ) || m_nIndirectDistance > RAY_FAR_AWAY )
            return RayJson::Fail( "distance must be more than 0 and at most 1000000" );
        if ( ! ( m_nIndirectStrength >= 0.0 ) || m_nIndirectStrength > 100.0 )
            return RayJson::Fail( "strength must be from 0 to 100" );
    }
    else
    {
        CSettings oDefault;

        SetIndirect( oDefault.m_nIndirectBounces, oDefault.m_nIndirectSamples, oDefault.m_nIndirectDistance,
                     oDefault.m_nIndirectStrength );
        SetIndirectMethod( oDefault.m_nIndirectMethod, oDefault.m_nIndirectObjects );
    }

    const IASLib::CJsonNode *pReserved = oNode.get( "reserved1997" );

    if ( pReserved == NULL )
        return TRUE;

    RayJson::CWhere oWhere( "reserved1997" );

    return RayJson::OnlyFields( *pReserved, astrReserved ) &&
         RayJson::GetBool( *pReserved, "gammaCorrect", m_bGammaCorrect, FALSE ) &&
         RayJson::GetNumber( *pReserved, "gammaCorrection", m_nGammaCorrection, FALSE ) &&
         RayJson::GetBool( *pReserved, "adaptiveDepth", m_bAdaptiveDepth, FALSE ) &&
         RayJson::GetBool( *pReserved, "antialias", m_bAntialias, FALSE ) &&
         RayJson::GetBool( *pReserved, "adaptiveAntialiasing", m_bAdaptiveAntialiasing, FALSE ) &&
         RayJson::GetNumber( *pReserved, "antialiasThreshold", m_nAntialiasThreshold, FALSE ) &&
         GetWord( *pReserved, "maximumAntialiasDepth", m_nMaximumAntialiasDepth, 0 ) &&
         RayJson::GetBool( *pReserved, "atmosphericEffects", m_bAtmosphericEffects, FALSE ) &&
         RayJson::GetBool( *pReserved, "distanceAttenuation", m_bDistanceAttenuation, FALSE ) &&
         RayJson::GetNumber( *pReserved, "distanceHazeHemidepth", m_nDistanceHazeHemidepth, FALSE ) &&
         RayJson::GetBool( *pReserved, "caustics", m_bCaustics, FALSE ) &&
         RayJson::GetBool( *pReserved, "adaptiveCaustics", m_bAdaptiveCaustics, FALSE ) &&
         GetWord( *pReserved, "maximumCausticDepth", m_nMaximumCausticDepth, 0 ) &&
         RayJson::GetNumber( *pReserved, "causticThreshold", m_nCausticThreshold, FALSE ) &&
         RayJson::GetBool( *pReserved, "simpleRadiosity", m_bSimpleRadiosity, FALSE ) &&
         RayJson::GetBool( *pReserved, "traceByColor", m_bTraceByColor, FALSE ) &&
         RayJson::GetBool( *pReserved, "focalBlur", m_bFocalBlur, FALSE ) &&
         RayJson::GetNumber( *pReserved, "depthOfField", m_nDepthOfField, FALSE ) &&
         RayJson::GetNumber( *pReserved, "focalDisplacement", m_nFocalDisplacement, FALSE ) &&
         RayJson::GetBool( *pReserved, "adaptiveFocalBlur", m_bAdaptiveFocalBlur, FALSE ) &&
         RayJson::GetNumber( *pReserved, "adaptiveFocalThreshold", m_nAdaptiveFocalThreshold, FALSE ) &&
         RayJson::GetBool( *pReserved, "fractalTurbulence", m_bFractalTurbulence, FALSE ) &&
         RayJson::GetNumber( *pReserved, "fractalSeed", m_nFractalSeed, FALSE );
}
