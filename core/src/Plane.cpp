/*************************************************************************
*   CPlane                                                               *
*                                                                        *
*       This class defines the behavior of an infinite, flat, infinitely *
* thin plane. It includes all of the intersection and shading methods.   *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
    #include "PlanePropertiesDlg.h"
#else
    #include "Globals.h"
#endif

#include "Plane.h"
#include "Light.h"
#include "SceneJson.h"

#ifdef RAY_GUI
#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#define PLANE_VERSION   ((DWORD)3)
#define VERSION_2       ((DWORD)2)
#define VERSION_1       ((DWORD)1)

CPlane::CPlane( void )
{
    CMaterial   *pMaterial;

    pMaterial = new CDiffuseMaterial;
    m_oMaterials.Add( pMaterial, 1.0 );

    m_oColor.Set( 0.5, 0.5, 0.5 );
}

CPlane::CPlane( FLT nX, FLT nY, FLT nZ, CVector oNormal ) :
m_oPosition( nX, nY, nZ ), m_oNormal( oNormal )
{
    CMaterial   *pMaterial;

    pMaterial = new CDiffuseMaterial;
    m_oMaterials.Add( pMaterial, 1.0 );

    m_oColor.Set( 0.5, 0.5, 0.5 );
}

CPlane::CPlane( CVector oPosition, CVector oNormal ) :
m_oPosition( oPosition ), m_oNormal( oNormal )
{
    CMaterial   *pMaterial;

    pMaterial = new CDiffuseMaterial;
    m_oMaterials.Add( pMaterial, 1.0 );

    m_oColor.Set( 0.5, 0.5, 0.5 );
}

CPlane::~CPlane( void )
{
}


#ifdef RAY_GUI

void CPlane::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hPlane;

    hPlane = pTree->AddObject( this, 10, "Plane", hUnder );
    CObjects::MatToTree( pTree, hPlane );
}

void CPlane::SetProperties( void )
{
    CPlanePropertiesDlg     oProp( &m_oPosition, &m_oNormal );
    int                     nCount;
    CMaterial              *pMaterial;

    oProp.m_oColor = m_oColor;
    if ( ( oProp.DoModal() == IDOK ) && ( oProp.m_bColorChanged ) )
    {
        m_oColor = oProp.m_oColor;
        nCount = 0;
        while ( nCount < m_oMaterials.GetSize() )
        {
            pMaterial = m_oMaterials[ nCount ];
            pMaterial->SetColor( m_oColor );
            nCount++;
        }
    }
}

#endif

DYNAMIC_IMPLEMENTS( CPlane, "plane" );


BOOL CPlane::Intersect( CRay &oRay, CVector &oIntersect )
{
    FLT     nA;
    FLT     nB;
    FLT     nC;
    FLT     nD;
    FLT     nDenom;
    FLT     nNumer;
    FLT     nT;

        // Here we develop the parametric equation for a plane
    nA = m_oNormal.GetX();
    nB = m_oNormal.GetY();
    nC = m_oNormal.GetZ();
    nD = -( (m_oNormal.GetX() * m_oPosition.GetX() ) + 
            (m_oPosition.GetY() * m_oNormal.GetY() ) +
            (m_oPosition.GetZ() * m_oNormal.GetZ() ) );

        // The solution is given by t= -(ax+by+cz+d)/(ai+bj+ck)

    nNumer = - ( (nA * oRay.GetX()) + ( nB * oRay.GetY() ) + ( nC * oRay.GetZ() ) + nD );
    nDenom = ( nA * oRay.GetI()) + ( nB * oRay.GetJ() ) + ( nC * oRay.GetK() );

    if ( nDenom == 0.0 )
        return FALSE;

    nT = nNumer / nDenom;

    if ( ( nT > 0.0 ) && ( nT < oRay.GetT() ) )
    {
        oRay.SetT( nT );
        oIntersect.Set( (oRay.GetI() * nT) + oRay.GetX(), 
                        (oRay.GetJ() * nT) + oRay.GetY(),
                        (oRay.GetK() * nT) + oRay.GetZ() );
        return TRUE;
    }

        // No intersection
    return FALSE;
}

CColor CPlane::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
    return BaseShade( oScene, oRay, oIntersect, m_oNormal );
}

CColor CPlane::Shade( CLight *pLight, CVector &oIntersect )
{
    CColor      oColor;
    CColor      oLight;
    FLT         nIncidentCos;
    CVector     oGourand;
    CVector     oToLight;
    CVector     oNormal;

    oToLight = pLight->GetPosition() - oIntersect;

    oLight = pLight->GetShade( oToLight );

    oToLight.Normalize();

    oNormal = m_oNormal;
    oNormal.Normalize();
    
    nIncidentCos = oNormal.Dot( oToLight );    // Planes are two sided
  

    if ( nIncidentCos >= 0.0 )
    {
        oGourand.Set( oLight.GetR(), oLight.GetG(), oLight.GetB() );

        oGourand = oGourand * nIncidentCos;
    }
    return oColor;
}

void CPlane::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "position", m_oPosition );
    RayJson::PutVector( oNode, "normal", m_oNormal );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
}

BOOL CPlane::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "position", "normal", "color", "materials", NULL };

    return RayJson::OnlyFields( oNode, astrFields ) &&
           RayJson::GetVector( oNode, "position", m_oPosition, TRUE ) &&
           RayJson::GetVector( oNode, "normal", m_oNormal, TRUE ) &&
           RayJson::GetColor( oNode, "color", m_oColor, FALSE ) &&
           m_oMaterials.FromJson( oNode, "materials" );
}

BOOL CPlane::SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV )
{
    CVector oNormal( m_oNormal );
    CVector oReference;

    oNormal.Normalize();

        // Two axes across the plane: u = normal x reference, v = u x normal,
        // with the reference +y, or +z for a plane facing (nearly) up or down.
    if ( ABS( oNormal.GetY() ) < 0.999 )
        oReference.Set( 0.0, 1.0, 0.0 );
    else
        oReference.Set( 0.0, 0.0, 1.0 );

    CVector oAxisU = oNormal.Cross( oReference );
    oAxisU.Normalize();
    CVector oAxisV = oAxisU.Cross( oNormal );
    CVector oOffset = oPoint - m_oPosition;

    nU = oOffset.Dot( oAxisU );
    nV = oOffset.Dot( oAxisV );
    return TRUE;
}
