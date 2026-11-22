#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "MetalMaterial.h"
#include "Scene.h"
#include "SceneJson.h"
#include "Sampling.h"

#include <math.h>

IMPLEMENT_MATERIAL( CMetalMaterial, Metal Material, "metal" );

namespace
{
    struct SMetal
    {
        const char *strName;
        FLT         anN[ 3 ];       // red, green, blue (about 650, 550, 450 nm)
        FLT         anK[ 3 ];
    };

        // Representative optical constants (Johnson and Christy; Rakic;
        // Palik), at about 650, 550 and 450 nm.
    const SMetal s_aoMetals[] =
    {
        { "gold",     { 0.166, 0.433, 1.381 }, { 3.15, 2.36, 1.83 } },
        { "silver",   { 0.053, 0.059, 0.052 }, { 4.41, 3.59, 2.66 } },
        { "copper",   { 0.214, 1.027, 1.175 }, { 3.67, 2.58, 2.40 } },
        { "aluminum", { 1.47,  0.958, 0.618 }, { 7.79, 6.69, 5.47 } },
        { "iron",     { 2.91,  2.93,  2.60  }, { 3.08, 2.93, 2.79 } },
        { "chromium", { 3.18,  3.04,  2.29  }, { 3.31, 3.32, 3.23 } },
        { "titanium", { 2.75,  2.54,  2.12  }, { 3.36, 3.21, 2.92 } },
        { "platinum", { 2.38,  2.13,  1.85  }, { 4.26, 3.71, 3.13 } },
    };
}

BOOL CMetalMaterial::Preset( const std::string &strName, FLT anN[ 3 ], FLT anK[ 3 ] )
{
    for ( const SMetal &oMetal : s_aoMetals )
    {
        if ( strName == oMetal.strName )
        {
            for ( int nChannel = 0; nChannel < 3; nChannel++ )
            {
                anN[ nChannel ] = oMetal.anN[ nChannel ];
                anK[ nChannel ] = oMetal.anK[ nChannel ];
            }
            return TRUE;
        }
    }
    return FALSE;
}

CMetalMaterial::CMetalMaterial( void ) :
m_strMetal( "aluminum" ),
m_bGivenN( FALSE ),
m_bGivenK( FALSE ),
m_bTinted( FALSE ),
m_oTint( 1.0, 1.0, 1.0 )
{
    Preset( m_strMetal, m_anN, m_anK );
    m_nRoughness.SetRange( 0.0, 1.0 );
    m_nRoughness = 0.0;
}

FLT CMetalMaterial::Reflectance( FLT nCos, FLT nN, FLT nK )
{
        // The exact Fresnel equations for a conductor (n + ik) in air.
    double dCos = ( nCos < 0.0 ) ? -(double)nCos : (double)nCos;

    if ( dCos > 1.0 )
        dCos = 1.0;

    double dSin2 = 1.0 - ( dCos * dCos );
    double dN2 = (double)nN * (double)nN;
    double dK2 = (double)nK * (double)nK;
    double dT = dN2 - dK2 - dSin2;
    double dRoot = sqrt( ( dT * dT ) + ( 4.0 * dN2 * dK2 ) );   // a^2 + b^2
    double dA = sqrt( (std::max)( 0.0, 0.5 * ( dRoot + dT ) ) );
    double dRs = ( dRoot - ( 2.0 * dA * dCos ) + ( dCos * dCos ) ) / ( dRoot + ( 2.0 * dA * dCos ) + ( dCos * dCos ) );
    double dSinTan = ( dCos > 0.0 ) ? dSin2 / dCos : 0.0;        // sin(theta) tan(theta)
    double dRp = dRs * ( dRoot - ( 2.0 * dA * dSinTan ) + ( dSinTan * dSinTan ) ) /
                       ( dRoot + ( 2.0 * dA * dSinTan ) + ( dSinTan * dSinTan ) );

    if ( dCos <= 0.0 )
        return 1.0;
    return (FLT)( 0.5 * ( dRs + dRp ) );
}

CColor CMetalMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
        // Light reaches a mirror only by reflection, which the other
        // GetColor traces.
    return CColor( 0, 0, 0 );
}

CColor CMetalMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CVector     oView( oIntersect.GetX() - oViewIncident.GetX(),
                       oIntersect.GetY() - oViewIncident.GetY(),
                       oIntersect.GetZ() - oViewIncident.GetZ() );
    CVector     oFacing( oNormal );

    oFacing.Normalize();
    oView.Normalize();
    if ( oFacing.Dot( oView ) > 0.0 )
        oFacing = oFacing * -1.0;

        // The mirror direction, or about a microfacet normal when rough.
    CVector     oMicro( oFacing );
    CMapPoint   oAt( oIntersect, pIntersect );
    FLT         nRoughness = m_nRoughness.At( oAt );

    if ( nRoughness > 0.0 )
    {
        FLT nU, nV;

        oViewIncident.SamplePoint( RaySampling::PURPOSE_REFLECT, nU, nV );

        CVector oSample = RaySampling::Microfacet( oFacing, nRoughness, nU, nV );
        CVector oStep = oSample * ( 2.0 * oSample.Dot( oView ) );
        CVector oRough = oView - oStep;

        if ( oRough.Dot( oFacing ) > 0.0 )
            oMicro = oSample;
    }

    CVector oTwice = oMicro * ( 2.0 * oMicro.Dot( oView ) );
    CVector oReflection = oView - oTwice;
    CRay    oReflect;
    CColor  oReflected;

    oReflect.CreateRay( oReflection, oIntersect, oViewIncident.GetInfluence(), oViewIncident.GetPixelX(), oViewIncident.GetPixelY() );
    oReflect.InheritSample( oViewIncident, RaySampling::PURPOSE_REFLECT );
    oReflect.SetT( 1e100 );
    oReflect.SetDepth( oViewIncident.GetDepth() );
    oScene.TraceRay( oReflect, oReflected, pIntersect, CRayCounts::REFLECTION );

    FLT     nCos = -oMicro.Dot( oView );
    CColor  oTint = ColorAt( m_oTint.At( oAt ), oIntersect, pIntersect );

    return CColor( oReflected.GetR() * Reflectance( nCos, m_anN[ 0 ], m_anK[ 0 ] ) * oTint.GetR(),
                   oReflected.GetG() * Reflectance( nCos, m_anN[ 1 ], m_anK[ 1 ] ) * oTint.GetG(),
                   oReflected.GetB() * Reflectance( nCos, m_anN[ 2 ], m_anK[ 2 ] ) * oTint.GetB() );
}

#ifdef RAY_GUI
void CMetalMaterial::Properties( void )
{
}

void CMetalMaterial::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddMaterial( this, 18, "Metal Material", hUnder );
}

void CMetalMaterial::ToList( CListCtrl &oList, int nIndex, FLT nInfluence )
{
    LVITEM      oItem;
    CString     strTemp;

    oItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    oItem.iItem = nIndex;
    oItem.iSubItem = 0;
    oItem.state = 0;
    oItem.stateMask = 0;
    oItem.pszText = "Metal";
    oItem.cchTextMax = 0;
    oItem.iImage = 2;
    oItem.lParam = (LPARAM)this;
    oList.InsertItem( &oItem );

    oItem.iSubItem = 1;
    oItem.mask = LVIF_TEXT;
    strTemp.Format( "%f", nInfluence );
    oItem.pszText = (LPSTR)(LPCSTR)strTemp;
    oList.SetItem( &oItem );
}
#endif

void CMetalMaterial::ToJson( IASLib::CObjectNode &oNode )
{
    if ( ! m_strMetal.empty() )
        oNode.put( "metal", m_strMetal.c_str() );
    if ( m_bGivenN || m_strMetal.empty() )
    {
        IASLib::CArrayNode *pN = oNode.putArray( "n" );

        pN->add( (double)m_anN[ 0 ] )->add( (double)m_anN[ 1 ] )->add( (double)m_anN[ 2 ] );
    }
    if ( m_bGivenK || m_strMetal.empty() )
    {
        IASLib::CArrayNode *pK = oNode.putArray( "k" );

        pK->add( (double)m_anK[ 0 ] )->add( (double)m_anK[ 1 ] )->add( (double)m_anK[ 2 ] );
    }
    if ( m_bTinted )
        m_oTint.Write( oNode, "tint" );
    if ( m_nRoughness.IsMapped() || m_nRoughness > 0.0 )
        m_nRoughness.Write( oNode, "roughness" );
}

BOOL CMetalMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "metal", "n", "k", "tint", "roughness", NULL };
    std::string strMetal = m_strMetal;

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetString( oNode, "metal", strMetal, FALSE ) ||
         ! m_oTint.Read( oNode, "tint", FALSE ) ||
         ! m_nRoughness.Read( oNode, "roughness", FALSE ) )
        return FALSE;
    if ( oNode.get( "tint" ) )
        m_bTinted = TRUE;
    if ( oNode.get( "metal" ) )
    {
        if ( ! Preset( strMetal, m_anN, m_anK ) )
            return RayJson::Fail( "unknown metal \"" + strMetal + "\" (gold, silver, copper, aluminum, iron, chromium, titanium or platinum)" );
        m_strMetal = strMetal;
    }
    else if ( oNode.get( "n" ) && oNode.get( "k" ) )
        m_strMetal.clear();

    const char *astrNames[] = { "n", "k" };

    for ( int nWhich = 0; nWhich < 2; nWhich++ )
    {
        const IASLib::CJsonNode *pValue = oNode.get( astrNames[ nWhich ] );
        FLT                     *pnTarget = ( nWhich == 0 ) ? m_anN : m_anK;

        if ( pValue == NULL )
            continue;

        RayJson::CWhere oWhere( astrNames[ nWhich ] );

        if ( ! pValue->isArray() || pValue->size() != 3 )
            return RayJson::Fail( "must be [red, green, blue]" );
        for ( int nChannel = 0; nChannel < 3; nChannel++ )
        {
            const IASLib::CJsonNode *pPart = pValue->get( nChannel );
            double                   dPart = pPart->isNumber() ? pPart->doubleValue() : -1.0;

            if ( ! ( dPart >= 0.0 ) || ! isfinite( dPart ) || ( nWhich == 0 && dPart <= 0.0 ) )
                return RayJson::Fail( ( nWhich == 0 ) ? "each n must be more than 0" : "each k must be 0 or more" );
            pnTarget[ nChannel ] = (FLT)dPart;
        }
        if ( nWhich == 0 )
            m_bGivenN = TRUE;
        else
            m_bGivenK = TRUE;
    }
    if ( m_strMetal.empty() && ( ! m_bGivenN || ! m_bGivenK ) )
        return RayJson::Fail( "a metal needs \"metal\" (a preset) or both \"n\" and \"k\"" );
    if ( ! m_nRoughness.IsMapped() && ( m_nRoughness < 0.0 || m_nRoughness > 1.0 ) )
        return RayJson::Fail( "roughness must be between 0 and 1" );
    return TRUE;
}
