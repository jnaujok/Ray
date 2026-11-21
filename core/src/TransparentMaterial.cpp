#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "TransparentMaterial.h"
#include "Scene.h"
#include "SceneJson.h"
#include "Sampling.h"
#include "Lens.h"
#include "Spectrum.h"

#include <math.h>
#include <algorithm>

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
m_nAttenuationStep( 0.05 ),
m_nRoughness( 0.0 ),
m_nAbbe( 0.0 )
{
    m_nIndexOfRefraction = 1.0;
    m_nIndexOfRefraction.SetRange( 0.001, 50.0 );
    m_nRoughness.SetRange( 0.0, 1.0 );
    m_nAttenuationDistance.SetRange( 1.0e-6, 1.0e9 );
}

CTransparentMaterial::CTransparentMaterial( CColor oColor, FLT nRefract ) :
m_oColor( oColor ),
m_oAttenuationColor( 1.0, 1.0, 1.0 ),
m_nAttenuationDistance( 0.0 ),
m_nAttenuationStep( 0.05 ),
m_nRoughness( 0.0 ),
m_nAbbe( 0.0 )
{
    m_nIndexOfRefraction = nRefract;
    m_nIndexOfRefraction.SetRange( 0.001, 50.0 );
    m_nRoughness.SetRange( 0.0, 1.0 );
    m_nAttenuationDistance.SetRange( 1.0e-6, 1.0e9 );
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

CColor CTransparentMaterial::VolumeTransmittance( const CVector &oFrom, const CVector &oDirection, FLT nLength,
                                                  const CObjects *pObject, FLT nJitter ) const
{
        // Beer-Lambert along the path: each step of length d at a point
        // whose color is c and distance D takes away -ln( c ) d / D, so a
        // constant texture gives exactly c^( length / D ), as the closed form
        // does. A channel of 0 is kept just above it (it would absorb all).
    int     nSteps = (int)ceil( (double)( nLength / m_nAttenuationStep ) );

    if ( nSteps < 1 )
        nSteps = 1;
    if ( nSteps > 100000 )
        nSteps = 100000;

    FLT     nStep = nLength / (FLT)nSteps;
    double  adDepth[ 3 ] = { 0.0, 0.0, 0.0 };
    CVector oStart( oFrom );
    CVector oWay( oDirection );

    for ( int nIndex = 0; nIndex < nSteps; nIndex++ )
    {
        CVector     oAlong( oWay * ( ( (FLT)nIndex + nJitter ) * nStep ) );
        CVector     oPoint( oStart + oAlong );
        CMapPoint   oAt = CMapPoint::InSpace( oPoint, pObject );
        CColor      oLeft( m_oAttenuationColor.At( oAt ) );
        FLT         nDistance = m_nAttenuationDistance.At( oAt );
        FLT         anLeft[ 3 ] = { oLeft.GetR(), oLeft.GetG(), oLeft.GetB() };

        if ( ! ( nDistance > 0.0 ) )
            continue;
        for ( int nChannel = 0; nChannel < 3; nChannel++ )
        {
            double dLeft = (std::min)( 1.0, (std::max)( 1.0e-9, (double)anLeft[ nChannel ] ) );

            adDepth[ nChannel ] -= log( dLeft ) * (double)nStep / (double)nDistance;
        }
    }
    return CColor( (FLT)exp( -adDepth[ 0 ] ), (FLT)exp( -adDepth[ 1 ] ), (FLT)exp( -adDepth[ 2 ] ) );
}

FLT CTransparentMaterial::IndexFor( int nChannel, FLT nBase ) const
{
    if ( ! ( m_nAbbe > 0.0 ) )
        return nBase;
    return CLens::DispersedIndex( nBase, m_nAbbe, nChannel );
}

CColor CTransparentMaterial::GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect )
{
        // Dispersive glass bends each wavelength its own way: a ray in every
        // channel goes on as three, red, green and blue, each keeping only
        // its own channel of what it sees. From here on each stays in its
        // channel, so further glass bends it by that channel's index too.
    if ( ( m_nAbbe > 0.0 ) && ( oViewIncident.GetChannel() == CLens::CHANNEL_ALL ) && oScene.GetCamera().IsSpectral() )
    {
            // Traced spectrally, it goes on as four single wavelengths, one
            // from each quarter of the range, each weighed by how its
            // wavelength looks (see Spectrum.h).
        FLT     anSum[ 3 ] = { 0.0, 0.0, 0.0 };
        FLT     nWhere = oViewIncident.WavelengthOffset();

        for ( int nPart = 0; nPart < RaySpectrum::PARTS; nPart++ )
        {
            FLT     nNanometers;
            FLT     anWeight[ 3 ];

            RaySpectrum::PartWeights( nPart, nWhere, nNanometers, anWeight );

            CRay    oChannelRay( oViewIncident );
            CVector oPoint( oIntersect );
            CVector oChannelNormal( oNormal );

            oChannelRay.SetChannel( CLens::WavelengthChannel( nNanometers ) );

            CColor  oSeen = GetColor( oScene, oChannelRay, oPoint, oChannelNormal, pIntersect );

            anSum[ 0 ] += oSeen.GetR() * anWeight[ 0 ];
            anSum[ 1 ] += oSeen.GetG() * anWeight[ 1 ];
            anSum[ 2 ] += oSeen.GetB() * anWeight[ 2 ];
        }
        return CColor( anSum[ 0 ] / RaySpectrum::PARTS, anSum[ 1 ] / RaySpectrum::PARTS, anSum[ 2 ] / RaySpectrum::PARTS );
    }
    if ( ( m_nAbbe > 0.0 ) && ( oViewIncident.GetChannel() == CLens::CHANNEL_ALL ) )
    {
        FLT anChannel[ 3 ];

        for ( int nChannel = CLens::CHANNEL_RED; nChannel <= CLens::CHANNEL_BLUE; nChannel++ )
        {
            CRay    oChannelRay( oViewIncident );
            CVector oPoint( oIntersect );
            CVector oChannelNormal( oNormal );

            oChannelRay.SetChannel( nChannel );

            CColor  oSeen = GetColor( oScene, oChannelRay, oPoint, oChannelNormal, pIntersect );

            anChannel[ nChannel - 1 ] = ( nChannel == CLens::CHANNEL_RED ) ? oSeen.GetR() :
                                        ( nChannel == CLens::CHANNEL_GREEN ) ? oSeen.GetG() : oSeen.GetB();
        }
        return CColor( anChannel[ 0 ], anChannel[ 1 ], anChannel[ 2 ] );
    }

    CMapPoint   oAt( oIntersect, pIntersect );
    FLT         nIndex = IndexFor( oViewIncident.GetChannel(), m_nIndexOfRefraction.At( oAt ) );
    FLT         nRoughness = m_nRoughness.At( oAt );
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
    const CObjects *pInside = oViewIncident.GetInside();
    BOOL        bLeaving = ( pInside == pIntersect || ( pInside && pIntersect && pInside->SameSurface( pIntersect ) ) ) ? TRUE : FALSE;
    BOOL        bPasses = Refract( oView, oNormal, nIndex, oTransmit );

        // Rough (frosted) glass bends the ray through a microfacet normal
        // drawn from around the surface's. One tilted past the ray, or that
        // would send it out the wrong side, is passed over for the clear
        // direction.
    if ( nRoughness > 0.0 )
    {
        FLT     nU;
        FLT     nV;
        CVector oRough;

        oViewIncident.SamplePoint( RaySampling::PURPOSE_REFRACT, nU, nV );

        CVector oMicro = RaySampling::Microfacet( oNormal, nRoughness, nU, nV );
        FLT     nSide = oNormal.Dot( oView );

        if ( ( oMicro.Dot( oView ) > 0.0 ) == ( nSide > 0.0 ) )
        {
            BOOL    bRoughPasses = Refract( oView, oMicro, nIndex, oRough );
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
    if ( bLeaving && IsVolume() )
    {
            // Absorption that varies inside: added up along the path from
            // where the ray went in (TRANSMIT_STEP before it started) to here,
            // read at points shifted within each step by the ray's sample, so
            // a pixel's rays read between each other's.
        CVector oInside( oIntersect.GetX() - oViewIncident.GetX(),
                         oIntersect.GetY() - oViewIncident.GetY(),
                         oIntersect.GetZ() - oViewIncident.GetZ() );
        FLT     nLength = oInside.Magnitude() + TRANSMIT_STEP;
        CVector oDirection( oInside );
        FLT     nJitter;
        FLT     nUnused;

        oDirection.Normalize();

        CVector oBack( oDirection * TRANSMIT_STEP );
        CVector oFrom( oViewIncident.GetX(), oViewIncident.GetY(), oViewIncident.GetZ() );

        oFrom = oFrom - oBack;
        oViewIncident.SamplePoint( RaySampling::PURPOSE_VOLUME, nJitter, nUnused );
        oTransparent = oTransparent * VolumeTransmittance( oFrom, oDirection, nLength, pIntersect, nJitter );
    }
    else if ( bLeaving && ( m_nAttenuationDistance > 0.0 ) )
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
    m_oColor.Write( oNode, "color" );
    m_nIndexOfRefraction.Write( oNode, "indexOfRefraction" );
    if ( IsVolume() || m_nAttenuationDistance > 0.0 )
    {
        m_oAttenuationColor.Write( oNode, "attenuationColor" );
        m_nAttenuationDistance.Write( oNode, "attenuationDistance" );
        if ( IsVolume() && m_nAttenuationStep != 0.05 )
            RayJson::PutNumber( oNode, "attenuationStep", m_nAttenuationStep );
    }
    if ( m_nRoughness.IsMapped() || m_nRoughness > 0.0 )
        m_nRoughness.Write( oNode, "roughness" );
    if ( m_nAbbe > 0.0 )
        RayJson::PutNumber( oNode, "abbe", m_nAbbe );
}

BOOL CTransparentMaterial::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "influence", "texture", "color", "indexOfRefraction",
                                              "attenuationColor", "attenuationDistance", "attenuationStep",
                                              "roughness", "abbe", NULL };

    if ( ! RayJson::OnlyFields( oNode, astrFields ) ||
         ! m_oColor.Read( oNode, "color", FALSE ) ||
         ! m_nIndexOfRefraction.Read( oNode, "indexOfRefraction", FALSE ) ||
         ! m_oAttenuationColor.Read( oNode, "attenuationColor", FALSE ) ||
         ! m_nAttenuationDistance.Read( oNode, "attenuationDistance", FALSE ) ||
         ! RayJson::GetNumber( oNode, "attenuationStep", m_nAttenuationStep, FALSE ) ||
         ! m_nRoughness.Read( oNode, "roughness", FALSE ) ||
         ! RayJson::GetNumber( oNode, "abbe", m_nAbbe, FALSE ) )
        return FALSE;
    if ( ! ( m_nAttenuationStep > 0.0 ) || m_nAttenuationStep > 1.0e6 )
        return RayJson::Fail( "attenuationStep must be more than 0" );
    if ( m_oAttenuationColor.IsMapped() && ! m_nAttenuationDistance.IsMapped() && ! ( m_nAttenuationDistance > 0.0 ) )
        return RayJson::Fail( "a textured attenuationColor needs an attenuationDistance of more than 0" );
    if ( ! m_nRoughness.IsMapped() && ( ( m_nRoughness < 0.0 ) || ( m_nRoughness > 1.0 ) ) )
        return RayJson::Fail( "roughness must be between 0 and 1" );
    if ( m_nAbbe < 0.0 )
        return RayJson::Fail( "abbe can't be negative (0 doesn't disperse)" );
    if ( ( m_nAbbe > 0.0 ) && ( m_nAbbe < 5.0 ) )
        return RayJson::Fail( "abbe must be 0 (no dispersion) or at least 5" );
    if ( ! m_nAttenuationDistance.IsMapped() && m_nAttenuationDistance < 0.0 )
        return RayJson::Fail( "attenuationDistance can't be negative" );

    CColor oLeft( m_oAttenuationColor.Get() );

    if ( m_oAttenuationColor.IsMapped() )
        return TRUE;
    if ( ( oLeft.GetR() < 0.0 ) || ( oLeft.GetR() > 1.0 ) ||
         ( oLeft.GetG() < 0.0 ) || ( oLeft.GetG() > 1.0 ) ||
         ( oLeft.GetB() < 0.0 ) || ( oLeft.GetB() > 1.0 ) )
        return RayJson::Fail( "attenuationColor must be between 0 and 1" );
    return TRUE;
}
