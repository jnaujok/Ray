/*************************************************************************
*   CObjects                                                             *
*                                                                        *
*       This is the base class for all of the three-dimensional objects, *
* including objects like the camera and lights.                          *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Scene.h"
#include "Light.h"
#include "Sampling.h"
#include "BumpMap.h"
#include "Texture.h"
#include "SceneJson.h"

#include <string.h>
#include <algorithm>
#include <vector>



CDynObject *CDynObject::m_pFirst = NULL;

static CDynObject   g_oBaseObject( NULL, "Base Object", NULL );

    // The registry of object classes. It is a static object, so it exists
    // in every program (it used to be created only by the GUI), and it reads
    // the registrations when it is used, so the order in which static
    // objects are built does not matter.
static CDynList g_oDynamicList;
CDynList    *g_pDynamicList = &g_oDynamicList;

CObjects::CObjects( void )
{
    m_bOwnsMaterials = TRUE;
    m_pShadeMaterials = NULL;
    m_pIncludedBy = NULL;
}

CObjects::~CObjects( void )
{
    CMaterial  *pMaterial;
    int         nIndex;

    nIndex = 0;
    while ( m_bOwnsMaterials && ( nIndex < m_oMaterials.GetSize() ) )
    {
        pMaterial = m_oMaterials[ nIndex ];
        delete pMaterial;
        nIndex++;
    }
    m_oMaterials.RemoveAll();
}


CObjects *CObjects::ReadJson( const IASLib::CJsonNode &oNode, BOOL bLights, const std::function<BOOL ( CObjects * )> &fnBeforeRead )
{
    std::string strType;

    if ( ! RayJson::GetString( oNode, "type", strType, TRUE ) )
        return NULL;

    CObjects *pObject = g_pDynamicList->Create( strType.c_str() );

    if ( pObject == NULL || pObject->IsCamera() || ( pObject->IsALight() ? TRUE : FALSE ) != bLights )
    {
        delete pObject;
        RayJson::Fail( std::string( bLights ? "unknown light type \"" : "unknown object type \"" ) + strType + "\"" );
        return NULL;
    }

    if ( fnBeforeRead && ! fnBeforeRead( pObject ) )
    {
        delete pObject;
        return NULL;
    }

    const IASLib::CJsonNode *pName = oNode.isObject() ? oNode.get( "name" ) : NULL;
    BOOL                     bRead;

    if ( pName )
    {
            // The name is the scene's, not the object's: it is read here and
            // the rest goes to the object.
        if ( ! pName->isTextual() )
        {
            delete pObject;
            RayJson::Fail( "name must be text" );
            return NULL;
        }
        pObject->m_strName = pName->textString();

        std::unique_ptr<IASLib::CJsonNode> pCopy( oNode.clone() );

        static_cast<IASLib::CObjectNode &>( *pCopy ).remove( "name" );
        bRead = pObject->FromJson( *pCopy );
    }
    else
        bRead = pObject->FromJson( oNode );
    if ( ! bRead )
    {
        delete pObject;
        return NULL;
    }
    return pObject;
}

void CObjects::TextureAt( CVector &oPoint, STexturePoint &oAt )
{
    oAt.oPoint = TextureSpace( oPoint );
    oAt.nU = oAt.nV = 0.0;
    oAt.bHasUV = SurfaceCoords( oPoint, oAt.nU, oAt.nV ) ? TRUE : FALSE;
    oAt.bHasColor = SurfaceColor( oPoint, oAt.oColor ) ? TRUE : FALSE;
    oAt.pSurface = this;
    oAt.oHit = oPoint;
}

void CObjects::BumpToJson( IASLib::CObjectNode &oNode ) const
{
    if ( m_pBump )
        m_pBump->ToJson( *oNode.putObject( "bump" ) );
}

BOOL CObjects::BumpFromJson( const IASLib::CJsonNode &oNode )
{
    const IASLib::CJsonNode *pBump = oNode.isObject() ? oNode.get( "bump" ) : NULL;

    m_pBump.reset();
    if ( pBump == NULL )
        return TRUE;

    RayJson::CWhere           oWhere( "bump" );
    std::shared_ptr<CBumpMap> pNew( new CBumpMap );

    if ( ! pNew->FromJson( *pBump ) )
        return FALSE;
    m_pBump = pNew;
    return TRUE;
}

CColor CObjects::BaseShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oSmoothNormal )
{
    CMaterialList  &oMaterials = m_pShadeMaterials ? *m_pShadeMaterials : m_oMaterials;
        // Where a material's influence is read, if a texture drives it.
    CMapPoint       oInfluenceAt( oIntersect, this );
        // A bump map tilts the normal every material shades with. The smooth
        // normal is left as it is (a plane's is its own member).
    CVector     oNormal( oSmoothNormal );

    if ( m_pBump )
        m_pBump->Perturb( *this, oIntersect, oNormal, oRay );

    CColor      oReturn;
    CColor      oColor;
    CRay        oLightRay;
    CVector     oToLight;
    CLight     *pLight;
    int         nLight;
    CMaterial  *pMaterial;
    int         nMaterial;
    CColor      oBlack( 0, 0, 0 );
    CColor      oLight;
        // A ray gathering bounced light (IndirectShade) wants only the light
        // the surface scatters diffusely: its diffuse materials lit, and
        // what glows (ambient); no highlights, reflections or refraction.
    BOOL        bGather = oRay.IsGather();
    BOOL        bHaze = ( oScene.GetHaze() != NULL );

    oReturn.Set( 0, 0, 0 );

        // First we do the shadings that require light to be effective
        // i.e. diffuse, specular, etc.
    nLight = 0;
    pLight = oScene.GetLight( nLight );

    while ( pLight )
    {
        if ( pLight->GetSampleCount() > 0 )
        {
            AreaLightShade( oScene, oRay, oIntersect, oNormal, pLight, nLight, oReturn );
            nLight++;
            pLight = oScene.GetLight( nLight );
            continue;
        }

        oToLight = pLight->ToLight( oIntersect );
        oLightRay.CreateRay( oToLight, oIntersect, 1.0, -1, -1 );
        oScene.CountRay( pLight->ShadowRayKind() );
        if ( ! oScene.Intersect( oLightRay, this ) )
        {
            nMaterial = 0;
            while ( nMaterial < oMaterials.GetSize() )
            {
                pMaterial = oMaterials[ nMaterial ];
                if ( oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) != 0.0 && ( ! bGather || pMaterial->LitBySky() ) )
                {
                        // Each material gets its own copies: materials adjust
                        // the normal and light vector (normalizing them, turning
                        // the normal toward the viewer), and those changes must
                        // not reach the next material, the next light, or the
                        // object (a plane's normal is its own member).
                    CVector oMaterialNormal( oNormal );
                    CVector oMaterialToLight( oToLight );
                    CVector oPoint( oIntersect );

                    oLight = pLight->ShadeAt( oIntersect, oToLight );
                    if ( bHaze )
                        oLight = oLight * oScene.LightThrough( oIntersect, oToLight );
                    oColor = pMaterial->GetColor( oScene, oRay, oLight, oMaterialToLight, oPoint, oMaterialNormal, this );
                    oReturn = oReturn + ( oColor * oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) );
                }
                nMaterial++;
            }
        }
        nLight++;
        pLight = oScene.GetLight( nLight );
    }

    const CEnvironment *pEnvironment = oScene.GetEnvironment();

    if ( pEnvironment && pEnvironment->GetSamples() > 0 )
        EnvironmentShade( oScene, oRay, oIntersect, oNormal, *pEnvironment, oReturn );

    if ( oRay.GetBounce() < oScene.GetSettings().GetIndirectBounces() )
        IndirectShade( oScene, oRay, oIntersect, oNormal, oReturn );

        // Do the intersection/shadings that do NOT require light
        // i.e. reflections, transparency, etc.

    nMaterial = 0;
    while ( nMaterial < oMaterials.GetSize() )
    {
        pMaterial = oMaterials[ nMaterial ];
        if ( oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) != 0.0 && ( ! bGather || pMaterial->Glows() ) )
        {
                // The ray's influence on its pixel, scaled by this material's
                // share: a reflected or refracted ray made from it carries that
                // on, so depthThreshold can stop rays that no longer matter.
                // (The color is scaled by the material's influence here, once;
                // it is not scaled by the ray's influence as well, which would
                // count it again at every bounce.)
            CRay    oMaterialRay( oRay );
            CVector oMaterialNormal( oNormal );
            CVector oPoint( oIntersect );

            oMaterialRay.SetInfluence( oRay.GetInfluence() * ABS( oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) ) );
            oColor = pMaterial->GetColor( oScene, oMaterialRay, oPoint, oMaterialNormal, this );
            oReturn = oReturn + ( oColor * oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) );
        }
        nMaterial++;
    }

    return oReturn;
}

void CObjects::AreaLightShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal, CLight *pLight, int nLight,
                               CColor &oReturn )
{
    CMaterialList  &oMaterials = m_pShadeMaterials ? *m_pShadeMaterials : m_oMaterials;
        // Where a material's influence is read, if a texture drives it.
    CMapPoint       oInfluenceAt( oIntersect, this );
        // An area light is shaded from several of its points, each with its
        // own shadow ray and an even share of its light. The points spread
        // from the one the ray's sample point picks, so over a pixel's rays
        // they cover the light evenly.
    int     nSamples = pLight->GetSampleCount();
    FLT     nShare = 1.0 / (FLT)nSamples;
    FLT     nFirstU;
    FLT     nFirstV;

    oRay.SamplePoint( (uint64_t)RaySampling::PURPOSE_LIGHT + (uint64_t)nLight, nFirstU, nFirstV );

    for ( int nSample = 0; nSample < nSamples; nSample++ )
    {
        FLT     nU;
        FLT     nV;
        CVector oPoint;
        CVector oFrom( oIntersect );
        CRay    oLightRay;

        RaySampling::Spread( nFirstU, nFirstV, nSample, nU, nV );
        if ( ! pLight->SampleAt( oFrom, nU, nV, oPoint ) )
            continue;

        CVector oToLight( oPoint - oIntersect );

        oLightRay.CreateRay( oToLight, oIntersect, 1.0, -1, -1 );
        oScene.CountRay( pLight->ShadowRayKind() );
        if ( oScene.Intersect( oLightRay, this ) )
            continue;

        CColor  oLight( pLight->ShadeAt( oIntersect, oToLight ) * nShare );

        if ( oScene.GetHaze() )
            oLight = oLight * oScene.LightThrough( oIntersect, oToLight );

        for ( int nMaterial = 0; nMaterial < oMaterials.GetSize(); nMaterial++ )
        {
            if ( oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) == 0.0 )
                continue;
            if ( oRay.IsGather() && ! oMaterials[ nMaterial ]->LitBySky() )
                continue;

            CVector oMaterialNormal( oNormal );
            CVector oMaterialToLight( oToLight );
            CVector oMaterialPoint( oIntersect );
            CColor  oColor = oMaterials[ nMaterial ]->GetColor( oScene, oRay, oLight, oMaterialToLight, oMaterialPoint,
                                                                  oMaterialNormal, this );

            oReturn = oReturn + ( oColor * oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) );
        }
    }
}

void CObjects::EnvironmentShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal,
                                 const CEnvironment &oEnvironment, CColor &oReturn )
{
    CMaterialList  &oMaterials = m_pShadeMaterials ? *m_pShadeMaterials : m_oMaterials;
        // Where a material's influence is read, if a texture drives it.
    CMapPoint       oInfluenceAt( oIntersect, this );
        // The sky's light for the way the surface faces (worked out once,
        // smooth), scaled by the share of the sky the point can see: that
        // many shadow rays toward it, spread by the cosine over the half on
        // the side the ray came from. It goes to the diffuse materials as a
        // light straight along the normal.
    int     nSamples = oEnvironment.GetSamples();
    int     nOpen = 0;
    FLT     nFirstU;
    FLT     nFirstV;
    CVector oUp( oNormal );
    CVector oView( oIntersect.GetX() - oRay.GetX(), oIntersect.GetY() - oRay.GetY(), oIntersect.GetZ() - oRay.GetZ() );
    CVector oFirst;
    CVector oSecond;

    oUp.Normalize();
    if ( oUp.Dot( oView ) > 0.0 )
        oUp = oUp * -1.0;
    RaySampling::Basis( oUp, oFirst, oSecond );
    oRay.SamplePoint( (uint64_t)RaySampling::PURPOSE_ENVIRONMENT, nFirstU, nFirstV );

    for ( int nSample = 0; nSample < nSamples; nSample++ )
    {
        FLT     nU;
        FLT     nV;
        FLT     nX;
        FLT     nY;

        RaySampling::Spread( nFirstU, nFirstV, nSample, nU, nV );
        RaySampling::ConcentricDisc( nU, nV, nX, nY );

        FLT     nRest = 1.0 - ( nX * nX ) - ( nY * nY );
        FLT     nUp = ( nRest > 0.0 ) ? SQRT( nRest ) : 0.0;
        CVector oAcross1( oFirst * nX );
        CVector oAcross2( oSecond * nY );
        CVector oAlong( oUp * nUp );
        CVector oPartway( oAcross1 + oAcross2 );
        CVector oDirection( oPartway + oAlong );
        CVector oToSky( oDirection * RAY_FAR_AWAY );
        CRay    oSkyRay;

        oSkyRay.CreateRay( oToSky, oIntersect, 1.0, -1, -1 );
        oScene.CountRay( CRayCounts::SHADOW_SKY );
        if ( ! oScene.Intersect( oSkyRay, this ) )
            nOpen++;
    }
    if ( nOpen == 0 )
        return;

    CColor  oSky( oEnvironment.IrradianceIn( oUp ) );
    CColor  oLight( oSky * ( (FLT)nOpen / (FLT)nSamples ) );

    for ( int nMaterial = 0; nMaterial < oMaterials.GetSize(); nMaterial++ )
    {
        if ( oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) == 0.0 || ! oMaterials[ nMaterial ]->LitBySky() )
            continue;

        CVector oMaterialNormal( oNormal );
        CVector oMaterialToLight( oUp );
        CVector oMaterialPoint( oIntersect );
        CColor  oColor = oMaterials[ nMaterial ]->GetColor( oScene, oRay, oLight, oMaterialToLight, oMaterialPoint,
                                                              oMaterialNormal, this );

        oReturn = oReturn + ( oColor * oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) );
    }
}

BOOL CObjects::GatherAlong( CScene &oScene, CRay &oRay, CVector &oFrom, CVector &oUp, CVector &oDirection, FLT nLength,
                            uint64_t nBranch, CObjects *pTarget, CColor &oBounced )
{
    CSettings  &oSettings = oScene.GetSettings();
    CVector     oReach( oDirection * nLength );
    CVector     oHit;
    CRay        oGather;

        // The ray reaches nLength at most: CreateRay keeps hits within the
        // length of its vector.
    oGather.CreateRay( oReach, oFrom, oRay.GetInfluence(), oRay.GetPixelX(), oRay.GetPixelY() );
    oGather.InheritSample( oRay, (uint64_t)RaySampling::PURPOSE_INDIRECT + nBranch );
    oGather.CopySampleIndex( oRay );
    oGather.SetDepth( oRay.GetDepth() );
    oGather.SetGather( oRay.GetBounce() + 1 );
    oGather.Increment();
    if ( oGather.GetDepth() > oSettings.GetMaximumDepth() )
        return FALSE;

    oScene.CountRay( CRayCounts::INDIRECT );

    CObjects   *pHit = oScene.ClosestHit( oGather, oHit, this );

        // The scene skips the object the ray leaves, but a closed one (a
        // sphere, a box) can light itself from inside: try it alone, from
        // just off the surface.
    if ( pHit != this && IsClosed() && ( pTarget == NULL || pTarget == this ) )
    {
        FLT     nScale = 1.0 + ABS( oFrom.GetX() ) + ABS( oFrom.GetY() ) + ABS( oFrom.GetZ() );
        CVector oNudge( oUp * ( 1e-7 * nScale ) );
        CVector oNudged( oFrom + oNudge );
        CRay    oSelf( oGather );
        CVector oSelfHit;

        oSelf.CreateRay( oReach, oNudged, oRay.GetInfluence(), oRay.GetPixelX(), oRay.GetPixelY() );
        if ( Intersect( oSelf, oSelfHit ) )
        {
            CVector oToSelf( oSelfHit - oFrom );
            CVector oToOther( oHit - oFrom );

            if ( pHit == NULL || oToSelf.Magnitude() < oToOther.Magnitude() )
            {
                pHit = this;
                oHit = oSelfHit;
            }
        }
    }
        // Light from a direction counts once, for the object the ray meets
        // first: a ray aimed at one object that meets another on the way
        // is that one's light, gathered when it is aimed at.
    if ( pHit == NULL || ( pTarget && pHit != pTarget ) )
        return FALSE;

    oBounced = pHit->Shade( oScene, oGather, oHit );

    const CHaze *pHaze = oScene.GetHaze();

    if ( pHaze )
    {
        CVector oPath( oHit - oFrom );

        oBounced = pHaze->Through( oBounced, pHaze->Transmittance( oFrom, oDirection, oPath.Magnitude() ) );
    }
    oBounced = oScene.GetCamera().ClampSample( oBounced );
    return TRUE;
}

    // A direction about the unit vector oUp from the point (nU, nV) of the
    // unit square, spread by the cosine (Malley's method: the concentric
    // disc lifted onto the half sphere).
static CVector CosineDirection( CVector &oUp, CVector &oFirst, CVector &oSecond, FLT nU, FLT nV )
{
    FLT     nX;
    FLT     nY;

    RaySampling::ConcentricDisc( nU, nV, nX, nY );

    FLT     nRest = 1.0 - ( nX * nX ) - ( nY * nY );
    FLT     nAlongUp = ( nRest > 0.0 ) ? SQRT( nRest ) : 0.0;
    CVector oAcross1( oFirst * nX );
    CVector oAcross2( oSecond * nY );
    CVector oAlong( oUp * nAlongUp );
    CVector oPartway( oAcross1 + oAcross2 );

    return oPartway + oAlong;
}

void CObjects::IndirectShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal, CColor &oReturn )
{
    CMaterialList  &oMaterials = m_pShadeMaterials ? *m_pShadeMaterials : m_oMaterials;
        // Where a material's influence is read, if a texture drives it.
    CMapPoint       oInfluenceAt( oIntersect, this );
    BOOL            bDiffuse = FALSE;

    for ( int nMaterial = 0; nMaterial < oMaterials.GetSize() && ! bDiffuse; nMaterial++ )
        bDiffuse = ( oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) != 0.0 && oMaterials[ nMaterial ]->LitBySky() );
    if ( ! bDiffuse )
        return;

    CSettings  &oSettings = oScene.GetSettings();
    int         nSamples = ( oRay.GetBounce() == 0 ) ? oSettings.GetIndirectSamples() : 1;
    FLT         nDistance = oSettings.GetIndirectDistance();
    CVector     oUp( oNormal );
    CVector     oRayDirection( oRay.GetI(), oRay.GetJ(), oRay.GetK() );
    CVector     oView( oIntersect.GetX() - oRay.GetX(), oIntersect.GetY() - oRay.GetY(), oIntersect.GetZ() - oRay.GetZ() );
    CVector     oFirst;
    CVector     oSecond;
        // The light reaching the point from around it, as the integral of
        // the light arriving from each direction times the cosine there.
    CColor      oIncoming( 0, 0, 0 );

    if ( nSamples < 1 )
        return;
    oUp.Normalize();
        // Whether the ray reached the surface from inside the object.
    BOOL        bFromInside = ( oUp.Dot( oRayDirection ) > 0.0 );

    if ( oUp.Dot( oView ) > 0.0 )
        oUp = oUp * -1.0;
    RaySampling::Basis( oUp, oFirst, oSecond );

    if ( oSettings.GetIndirectMethod() == CSettings::INDIRECT_HEMISPHERE )
    {
            // Gathering over the whole half above the point: rays spread by
            // the cosine, each reaching nDistance, from the ray's sample
            // point, so a stratified sampler covers the half evenly over a
            // pixel's rays. Each one's light counts pi / n.
        FLT nFirstU;
        FLT nFirstV;

        oRay.SamplePoint( (uint64_t)RaySampling::PURPOSE_INDIRECT, nFirstU, nFirstV );
        for ( int nSample = 0; nSample < nSamples; nSample++ )
        {
            FLT     nU;
            FLT     nV;
            CColor  oBounced;

            RaySampling::Spread( nFirstU, nFirstV, nSample, nU, nV );

            CVector oDirection( CosineDirection( oUp, oFirst, oSecond, nU, nV ) );

            if ( GatherAlong( oScene, oRay, oIntersect, oUp, oDirection, nDistance, (uint64_t)nSample, NULL, oBounced ) )
                oIncoming = oIncoming + ( oBounced * ( PI / (FLT)nSamples ) );
        }
    }
    else
    {
            // Aimed at the objects near the point, each treated as a light:
            // the objects whose bounds come within nDistance (by the scene's
            // tree), the "objects" setting's worth of them that cover the
            // most of the point's view, each gets nSamples rays aimed at it.
            // A ray counts only if that object is the first thing it meets,
            // so no direction counts twice, and each counts by the cosine at
            // the point and the share of the view it stands for, so the sum
            // is right whatever the number of rays.
        enum { AIM_CONE = 0, AIM_HALF = 1, AIM_DISC = 2 };
        struct STarget
        {
            int         nObject;            // -1: the object itself, from inside
            CObjects   *pObject;
            int         nAim;
            FLT         nSolidAngle;
            CVector     oCenter;            // cone: the bounding sphere's center; disc: its center on the plane
            CVector     oNormal;            // disc: the plane's normal
            FLT         nRadius;            // cone: the bounding sphere's; disc: the disc's
            FLT         nCenterDistance;
        };
        std::vector<int>        anNearby;
        std::vector<STarget>    aoTargets;
        CVector                 oAt( oIntersect );

        oScene.Nearby( oAt, nDistance, anNearby );
        for ( size_t nIndex = 0; nIndex < anNearby.size(); nIndex++ )
        {
            STarget     oTarget;
            CObjects   *pObject = oScene.GetObjectAt( anNearby[ nIndex ] );
            CVector     oMinimum;
            CVector     oMaximum;
            CVector     oPlanePoint;
            CVector     oPlaneNormal;

            if ( pObject == NULL || pObject == this )
                continue;
            oTarget.nObject = anNearby[ nIndex ];
            oTarget.pObject = pObject;
            if ( pObject->GetBounds( oMinimum, oMaximum ) )
            {
                CVector oSum( oMinimum + oMaximum );
                CVector oSpan( oMaximum - oMinimum );

                oTarget.oCenter = oSum * 0.5;
                oTarget.nRadius = oSpan.Magnitude() * 0.5;

                CVector oToCenter( oTarget.oCenter - oAt );

                oTarget.nCenterDistance = oToCenter.Magnitude();
                if ( oTarget.nCenterDistance <= oTarget.nRadius * 1.000001 )
                {
                    oTarget.nAim = AIM_HALF;
                    oTarget.nSolidAngle = 2.0 * PI;
                }
                else
                {
                    FLT nSin = oTarget.nRadius / oTarget.nCenterDistance;
                    FLT nCos = SQRT( 1.0 - ( nSin * nSin ) );

                        // Wholly below the point's horizon: nothing to gather.
                    if ( oUp.Dot( oToCenter ) < - nSin * oTarget.nCenterDistance )
                        continue;
                    oTarget.nAim = AIM_CONE;
                    oTarget.nSolidAngle = 2.0 * PI * ( 1.0 - nCos );
                }
            }
            else if ( pObject->PlaneOf( oPlanePoint, oPlaneNormal ) )
            {
                CVector oOffset( oAt - oPlanePoint );

                oPlaneNormal.Normalize();

                FLT     nHeight = oPlaneNormal.Dot( oOffset );
                FLT     nAbove = ABS( nHeight );

                if ( nAbove < 1e-9 * ( 1.0 + oAt.Magnitude() ) || nAbove >= nDistance )
                    continue;

                CVector oDown( oPlaneNormal * nHeight );

                oTarget.nAim = AIM_DISC;
                oTarget.oCenter = oAt - oDown;
                oTarget.oNormal = oPlaneNormal;
                oTarget.nRadius = SQRT( ( nDistance * nDistance ) - ( nHeight * nHeight ) );
                oTarget.nCenterDistance = nAbove;
                oTarget.nSolidAngle = 2.0 * PI * ( 1.0 - ( nAbove / nDistance ) );
            }
            else
                continue;
            aoTargets.push_back( oTarget );
        }
        if ( bFromInside && IsClosed() )
        {
            STarget     oSelf;
            CVector     oMinimum;
            CVector     oMaximum;

            if ( GetBounds( oMinimum, oMaximum ) )
            {
                CVector oSpan( oMaximum - oMinimum );

                oSelf.nObject = -1;
                oSelf.pObject = this;
                oSelf.nAim = AIM_HALF;
                oSelf.nSolidAngle = 2.0 * PI;
                oSelf.nRadius = 0.0;
                oSelf.nCenterDistance = oSpan.Magnitude();
                aoTargets.push_back( oSelf );
            }
        }
            // The ones that cover the most of the view first (in scene order
            // where they tie), as many as "objects" allows.
        std::stable_sort( aoTargets.begin(), aoTargets.end(),
                          []( const STarget &oA, const STarget &oB ) { return oA.nSolidAngle > oB.nSolidAngle; } );
        if ( (int)aoTargets.size() > oSettings.GetIndirectObjects() )
            aoTargets.resize( (size_t)oSettings.GetIndirectObjects() );

        for ( size_t nTarget = 0; nTarget < aoTargets.size(); nTarget++ )
        {
            STarget    &oTarget = aoTargets[ nTarget ];
            uint64_t    nStream = ( (uint64_t)( oTarget.nObject + 1 ) << 20 );
            FLT         nFirstU;
            FLT         nFirstV;
            CVector     oAxis;
            CVector     oAcrossA;
            CVector     oAcrossB;
            FLT         nCosLimit = 1.0;

            oRay.SamplePoint( (uint64_t)RaySampling::PURPOSE_INDIRECT + nStream, nFirstU, nFirstV );
                // A bounding sphere fills a cone of directions from the
                // point, and so does the disc of a plane within nDistance
                // (about the line straight down to the plane).
            if ( oTarget.nAim != AIM_HALF )
            {
                CVector oToCenter( oTarget.oCenter - oAt );

                oAxis = oToCenter * ( 1.0 / oTarget.nCenterDistance );
                RaySampling::Basis( oAxis, oAcrossA, oAcrossB );
                nCosLimit = 1.0 - ( oTarget.nSolidAngle / ( 2.0 * PI ) );
            }

                // An object whose bounds hold the point (a room, a large
                // mesh around it) is gathered over the whole half above the
                // point, not a small cone, so its few rays are spread far
                // thinner and one bright patch caught or missed shows as a
                // speckle. It gets four times the rays.
            int nRays = ( oTarget.nAim == AIM_HALF ) ? nSamples * 4 : nSamples;

            for ( int nSample = 0; nSample < nRays; nSample++ )
            {
                FLT     nU;
                FLT     nV;
                CColor  oBounced;

                RaySampling::Spread( nFirstU, nFirstV, nSample, nU, nV );
                if ( oTarget.nAim == AIM_HALF )
                {
                    CVector oDirection( CosineDirection( oUp, oFirst, oSecond, nU, nV ) );
                    FLT     nReach = oTarget.nCenterDistance + oTarget.nRadius;

                    if ( GatherAlong( oScene, oRay, oIntersect, oUp, oDirection, nReach * 1.0001 + 1e-9, nStream + (uint64_t)nSample,
                                      oTarget.pObject, oBounced ) )
                        oIncoming = oIncoming + ( oBounced * ( PI / (FLT)nRays ) );
                }
                else
                {
                        // Evenly over the cone: each direction stands for an
                        // equal share of its solid angle, so no direction
                        // counts for more than that, however near the surface.
                    FLT     nCos = 1.0 - ( nU * ( 1.0 - nCosLimit ) );
                    FLT     nSinSquared = 1.0 - ( nCos * nCos );
                    FLT     nSin = ( nSinSquared > 0.0 ) ? SQRT( nSinSquared ) : 0.0;
                    FLT     nTurn = 2.0 * PI * nV;
                    CVector oAlong( oAxis * nCos );
                    CVector oSide1( oAcrossA * ( nSin * (FLT)cos( (double)nTurn ) ) );
                    CVector oSide2( oAcrossB * ( nSin * (FLT)sin( (double)nTurn ) ) );
                    CVector oPartway( oAlong + oSide1 );
                    CVector oDirection( oPartway + oSide2 );
                    FLT     nFacing = oDirection.Dot( oUp );

                    FLT     nReach = ( oTarget.nAim == AIM_DISC ) ? oTarget.nCenterDistance / ( nCos > 1e-12 ? nCos : 1e-12 )
                                                                  : oTarget.nCenterDistance + oTarget.nRadius;

                    if ( nFacing <= 0.0 )
                        continue;
                    if ( GatherAlong( oScene, oRay, oIntersect, oUp, oDirection, nReach * 1.0001, nStream + (uint64_t)nSample,
                                      oTarget.pObject, oBounced ) )
                        oIncoming = oIncoming + ( oBounced * ( nFacing * oTarget.nSolidAngle / (FLT)nSamples ) );
                }
            }
        }
    }

        // A diffuse surface lit by incoming light E (the integral above)
        // sends out E / pi; as a light straight along the normal that is a
        // light of E / pi.
    CColor  oLight( oIncoming * ( oSettings.GetIndirectStrength() / PI ) );

    for ( int nMaterial = 0; nMaterial < oMaterials.GetSize(); nMaterial++ )
    {
        if ( oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) == 0.0 || ! oMaterials[ nMaterial ]->LitBySky() )
            continue;

        CVector oMaterialNormal( oNormal );
        CVector oMaterialToLight( oUp );
        CVector oMaterialPoint( oIntersect );
        CColor  oColor = oMaterials[ nMaterial ]->GetColor( oScene, oRay, oLight, oMaterialToLight, oMaterialPoint,
                                                              oMaterialNormal, this );

        oReturn = oReturn + ( oColor * oMaterials.InfluenceAt( nMaterial, oInfluenceAt ) );
    }
}

CDynObject::CDynObject( CObjects *(CFUNC *lpCreateEmpty)(void), LPCSTR strKey, LPCSTR strJsonType )
{
    m_lpCreateEmpty = lpCreateEmpty;
    m_strJsonType = strJsonType;
    m_strObjectID = strKey;
    m_pNext = m_pFirst;
    m_pFirst = this;
}

CDynObject::~CDynObject( void )
{
    CDynObject  *pLast;
    CDynObject  *pTemp;

    pLast = NULL;
    pTemp = m_pFirst;

    while ( pTemp != this )
    {
        pLast = pTemp;
        pTemp = pTemp->m_pNext;
    }

    if ( pLast == NULL )
        m_pFirst = m_pNext;
    else
        pLast->m_pNext = m_pNext;
}


int CDynObject::Compare( CDynObject *pCompare )
{
    return m_strObjectID.Compare( pCompare->m_strObjectID );
}

BOOL CDynObject::Match( LPCSTR lpszCompare )
{
    if ( m_strObjectID.Compare( lpszCompare ) == 0 )
        return TRUE;
    return FALSE;
}

CObjects *CDynObject::MakeEmpty( void )
{
    return m_lpCreateEmpty ? (*m_lpCreateEmpty)() : NULL;
}

CDynObject *CDynObject::GetFirst( void )
{
    return m_pFirst;
}

CDynObject *CDynObject::GetNext( void )
{
    return m_pNext;
}

CDynList::CDynList( void )
{
        // Retrieve the first node of the linked list
    m_pFirst = g_oBaseObject.GetFirst();
}

CDynList::~CDynList( void )
{
}

CObjects *CDynList::Create( LPCSTR strJsonType )
{
    for ( CDynObject *pObject = g_oBaseObject.GetFirst(); pObject; pObject = pObject->GetNext() )
    {
        if ( pObject->GetJsonType() && strJsonType && strcmp( pObject->GetJsonType(), strJsonType ) == 0 )
            return pObject->MakeEmpty();
    }
    return NULL;
}

LPCSTR CDynList::JsonTypeOf( LPCSTR strClassType )
{
    for ( CDynObject *pObject = g_oBaseObject.GetFirst(); pObject; pObject = pObject->GetNext() )
    {
        if ( pObject->Match( strClassType ) )
            return pObject->GetJsonType();
    }
    return NULL;
}

#ifdef __ENGINE__
#define ASSERT_ME
#endif

#ifdef __THROWER__
#define ASSERT_ME
#endif

#ifdef ASSERT_ME
#include <stdio.h>
void ASSERT_ShowError( LPCSTR lpszAssertion, LPCSTR lpszFileName, int nLineNumber )
{
    printf( "Assertion Failed:\n%[%s]\nFile: %s\nLine: %d\n\nBreak into code?", lpszAssertion, lpszFileName, nLineNumber );
    exit(-1);
}
#endif

