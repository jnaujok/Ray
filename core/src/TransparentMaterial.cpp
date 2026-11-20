#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "TransparentMaterial.h"
#include "Scene.h"
#include "SceneJson.h"
#include "Sampling.h"

#include <math.h>

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TRANSPARENT_MATERIAL_VERSION    ((DWORD)1)

IMPLEMENT_MATERIAL( CTransparentMaterial, Transparent Material, "transparent" );

    // How far past a surface a ray through it starts, so it doesn't hit
    // that surface again.
#define TRANSMIT_STEP   0.01

CTransparentMaterial::CTransparentMaterial( void ) :
m_oColor( 0.5, 0.5, 0.5 ),
m_oAttenuationColor( 1.0, 1.0, 1.0 ),
m_nAttenuationDistance( 0.0 ),
m_nRoughness( 0.0 )
{
    m_nIndexOfRefraction = 1.0;
}

CTransparentMaterial::CTransparentMaterial( CColor oColor, FLT nRefract ) :
m_oColor( oColor ),
m_oAttenuationColor( 1.0, 1.0, 1.0 ),
m_nAttenuationDistance( 0.0 ),
m_nRoughness( 0.0 )
{
    m_nIndexOfRefraction = nRefract;
}

CColor CTransparentMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CColor      oReturn( 0, 0, 0 );

    return oReturn;
}

BOOL CTransparentMaterial::Refract( CVector oView, CVector oNormal, FLT nIndex, CVector &oTransmit )
{
    FLT         nIOR;
    FLT         nCosTheta1;
    FLT         nCosTheta2;
    CVector     oStep;

    oNormal.Normalize();
    oView.Normalize();

        // Snell's law in vector form. With the normal turned against the
        // ray (so cos1 = -N.V is positive) and eta = n1 / n2:
        //   cos2^2 = 1 - eta^2 ( 1 - cos1^2 )
        //   T = eta V + ( eta cos1 - cos2 ) N
        // Entering the glass (the ray against the outward normal), eta is
        // 1 / n; leaving it, n.
    nCosTheta1 = -oNormal.Dot( oView );

    if ( nCosTheta1 >= 0.0 )
        nIOR = 1.0 / nIndex;
    else
    {
        nIOR = nIndex;
        oNormal = oNormal * -1.0;
        nCosTheta1 = -nCosTheta1;
    }

    nCosTheta2 = 1.0 - ( nIOR * nIOR * ( 1.0 - ( nCosTheta1 * nCosTheta1 ) ) );

    if ( nCosTheta2 < 0.0 )
    {
            // Total internal reflection: past the critical angle, inside the
            // denser glass, nothing gets out and the ray reflects.
        oStep = oNormal * ( 2.0 * nCosTheta1 );
        oTransmit = oView + oStep;
        return FALSE;
    }

    CVector oViewStep = oView * nIOR;

    nCosTheta2 = SQRT( nCosTheta2 );
    oStep = oNormal * ( ( nIOR * nCosTheta1 ) - nCosTheta2 );
    oTransmit = oViewStep + oStep;
    return TRUE;
}

CColor CTransparentMaterial::Attenuation( FLT nDistance ) const
{
    CColor  oLeft( m_oAttenuationColor );
    FLT     nPower = nDistance / m_nAttenuationDistance;

    return CColor( (FLT)pow( (double)oLeft.GetR(), (double)nPower ),
                   (FLT)pow( (double)oLeft.GetG(), (double)nPower ),
                   (FLT)pow( (double)oLeft.GetB(), (double)nPower ) );
}

CColor CTransparentMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
    CColor      oTransparent;
    CVector     oView( oIntersect.GetX() - oViewIncident.GetX(),
                       oIntersect.GetY() - oViewIncident.GetY(),
                       oIntersect.GetZ() - oViewIncident.GetZ() );
    CRay        oTransmitted;
    CVector     oTransmit;
    CVector     oTemp;
    CVector     oTransStep;

    oNormal.Normalize();

    oView.Normalize();

        // Whether the ray reached this surface from inside the object: it
        // went in through this material (or was turned back inside by it).
    BOOL        bLeaving = ( oViewIncident.GetInside() == pIntersect ) ? TRUE : FALSE;
    BOOL        bPasses = Refract( oView, oNormal, m_nIndexOfRefraction, oTransmit );

        // Rough (frosted) glass bends the ray through a microfacet normal
        // drawn from around the surface's. One tilted past the ray, or that
        // would send it out the wrong side, is passed over for the clear
        // direction.
    if ( m_nRoughness > 0.0 )
    {
        FLT     nU;
        FLT     nV;
        CVector oRough;

        oViewIncident.SamplePoint( RaySampling::PURPOSE_REFRACT, nU, nV );

        CVector oMicro = RaySampling::Microfacet( oNormal, m_nRoughness, nU, nV );
        FLT     nSide = oNormal.Dot( oView );

        if ( ( oMicro.Dot( oView ) > 0.0 ) == ( nSide > 0.0 ) )
        {
            BOOL    bRoughPasses = Refract( oView, oMicro, m_nIndexOfRefraction, oRough );
            FLT     nOut = oNormal.Dot( oRough );

                // Through the surface the ray keeps going the way it was;
                // reflected inside, it turns back.
            if ( bRoughPasses ? ( ( nOut > 0.0 ) == ( nSide > 0.0 ) ) : ( ( nOut > 0.0 ) != ( nSide > 0.0 ) ) )
            {
                oTransmit = oRough;
                bPasses = bRoughPasses;
            }
        }
    }

    oTransStep = oTransmit * TRANSMIT_STEP;

    oTemp = oIntersect + oTransStep;

    oTransmitted.CreateRay( oTransmit, oTemp, oViewIncident.GetInfluence(), oViewIncident.GetPixelX(), oViewIncident.GetPixelY() );

        // The above function is tailored for Shadow rays, so we need to stretch
        // out the valid intersection range.

    oTransmitted.InheritSample( oViewIncident, RaySampling::PURPOSE_REFRACT );
    oTransmitted.SetT( 1e100 );
    oTransmitted.SetDepth( oViewIncident.GetDepth() );
        // Passing through the surface changes sides; reflecting inside it
        // (past the critical angle) keeps the ray where it was.
    oTransmitted.SetInside( ( bLeaving != bPasses ) ? pIntersect : NULL );

    oScene.TraceRay( oTransmitted, oTransparent, NULL );

    oTransparent = oTransparent * ColorAt( m_oColor, oIntersect, pIntersect );

        // Coming out, the light this ray carries crossed the object from
        // where the ray went in (it started TRANSMIT_STEP past there).
    if ( bLeaving && ( m_nAttenuationDistance > 0.0 ) )
    {
        CVector oInside( oIntersect.GetX() - oViewIncident.GetX(),
                         oIntersect.GetY() - oViewIncident.GetY(),
                         oIntersect.GetZ() - oViewIncident.GetZ() );

        oTransparent = oTransparent * Attenuation( oInside.Magnitude() + TRANSMIT_STEP );
    }

    return oTransparent;
}


#ifdef RAY_GUI
#include "resource.h"
#include "TransparentMatDlg.h"

void CTransparentMaterial::Properties( void )
{
    CTransparentMatDlg       oDlg;

    oDlg.m_oColor = m_oColor;
    oDlg.m_nIndexOfRefraction = m_nIndexOfRefraction;

    if ( oDlg.DoModal() == IDOK )
    {
        m_oColor = oDlg.m_oColor;
        m_nIndexOfRefraction = oDlg.m_nIndexOfRefraction;
    }
}

void  CTransparentMaterial::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    pTree->AddMaterial( this, 20, "Transparent Material", hUnder );
}

void CTransparentMaterial::ToList( CListCtrl &oList, int nIndex, FLT nInfluence )
{
    LVITEM      oItem;
    CString     strTemp;

    oItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    oItem.iItem = nIndex;
    oItem.iSubItem = 0;
    oItem.state = 0;
    oItem.stateMask = 0;
    oItem.pszText = "Transparent";
    oItem.cchTextMax = 0;
    oItem.iImage = 3;
    oItem.lParam = (LPARAM)this;
    oList.InsertItem( &oItem );

    oItem.iSubItem = 1;
    oItem.mask = LVIF_TEXT;
    strTemp.Format( "%f", nInfluence );
    oItem.pszText = (LPSTR)(LPCSTR)strTemp;
    oList.SetItem( &oItem );
}

#endif

void CTransparentMaterial::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutColor( oNode, "color", m_oColor );
    RayJson::PutNumber( oNode, "indexOfRefraction", m_nIndexOfRefraction );
    if ( m_nAttenuationDistance > 0.0 )
    {
        RayJson::PutColor( oNode, "attenuationColor", m_oAttenuationColor );
        RayJson::PutNumber( oNode, "attenuationDistance", m_nAttenuationDistance );
    }
    if ( m_nRoughness > 0.0 )
        RayJson::PutNumber( oNode, "roughness", m_nRoughness );
}

BOOL CTransparentMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", "indexOfRefraction",
                                              "attenuationColor", "attenuationDistance", "roughness", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! RayJson::GetColor( oNode, "color", m_oColor, FALSE ) ||
         ! RayJson::GetNumber( oNode, "indexOfRefraction", m_nIndexOfRefraction, FALSE ) ||
         ! RayJson::GetColor( oNode, "attenuationColor", m_oAttenuationColor, FALSE ) ||
         ! RayJson::GetNumber( oNode, "attenuationDistance", m_nAttenuationDistance, FALSE ) ||
         ! RayJson::GetNumber( oNode, "roughness", m_nRoughness, FALSE ) )
        return FALSE;
    if ( ( m_nRoughness < 0.0 ) || ( m_nRoughness > 1.0 ) )
        return RayJson::Fail( "roughness must be between 0 and 1" );
    if ( m_nAttenuationDistance < 0.0 )
        return RayJson::Fail( "attenuationDistance can't be negative" );

    CColor oLeft( m_oAttenuationColor );

    if ( ( oLeft.GetR() < 0.0 ) || ( oLeft.GetR() > 1.0 ) ||
         ( oLeft.GetG() < 0.0 ) || ( oLeft.GetG() > 1.0 ) ||
         ( oLeft.GetB() < 0.0 ) || ( oLeft.GetB() > 1.0 ) )
        return RayJson::Fail( "attenuationColor must be between 0 and 1" );
    return TRUE;
}
