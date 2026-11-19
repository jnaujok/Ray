/*************************************************************************
*   CTriangle                                                              *
*                                                                        *
*       This class defines the behavior of the basic sphere. It handles  *
* all of the sphere intersection and shading information.                *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
    #include "resource.h"
//    #include "SpherePropertiesDlg.h"
#else
    #include "Globals.h"
#endif

#include "Triangle.h"
#include "Light.h"

#ifdef RAY_GUI
#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#define TRIANGLE_VERSION    ((DWORD)1)
                  
CTriangle::CTriangle( void )
{
    CMaterial   *pMaterial;

    pMaterial = new CDiffuseMaterial;
    m_oMaterials.Add( pMaterial, 1.0 );
    m_oColor.Set( 0.5, 0.5, 0.5 );
    m_oCorner1.Set( 0.0, 1.0, 0.0 );
    m_oCorner2.Set( -1.0, 0.0, 0.0 );
    m_oCorner3.Set( 1.0, 0.0, 0.0 );
    m_oSurfaceNormal.Set( 0.0, 0.0, -1.0 );
    m_oNormal1.Set( 0.0, 0.0, -1.0 );
    m_oNormal2.Set( 0.0, 0.0, -1.0 );
    m_oNormal3.Set( 0.0, 0.0, -1.0 );
}

CTriangle::CTriangle( CVector oVertex1,
                      CVector oNormal1,
                      CVector oVertex2,
                      CVector oNormal2,
                      CVector oVertex3,
                      CVector oNormal3 )
{
    CVector     oTemp1;
    CVector     oTemp2;

    oTemp1 = oVertex2 - oVertex1;
    oTemp2 = oVertex2 - oVertex3;

//    m_oSurfaceNormal = oTemp1.Cross( oTemp2 );

    m_oCorner1 = oVertex1;
    m_oCorner2 = oVertex2;
    m_oCorner3 = oVertex3;

    m_oNormal1 = oNormal1;
    m_oNormal2 = oNormal2;
    m_oNormal3 = oNormal3;

    m_oColor.Set( 0.5, 0.5, 0.5 );
}

CTriangle::~CTriangle( void )
{
}


#ifdef RAY_GUI
void CTriangle::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hTriangle;

    hTriangle = pTree->AddObject( this, 24, "Triangle", hUnder );
    CObjects::MatToTree( pTree, hTriangle );
}

void CTriangle::SetProperties( void )
{
/*    CTrianglePropertiesDlg    oProp( &m_oCenter, &m_nRadius );
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
*/
}
#endif

DYNAMIC_IMPLEMENTS( CTriangle );

CRayStream &operator >>( CRayStream &oStream, CTriangle &oTriangle )
{
    DWORD       dwVersion;
    CColor      oColor;

    oStream >> dwVersion;

    switch( dwVersion )
    {
        case TRIANGLE_VERSION:
            oStream >> oTriangle.m_oCorner1;
            oStream >> oTriangle.m_oCorner2;
            oStream >> oTriangle.m_oCorner3;
            oStream >> oTriangle.m_oSurfaceNormal;
            oStream >> oTriangle.m_oNormal1;
            oStream >> oTriangle.m_oNormal2;
            oStream >> oTriangle.m_oNormal3;
            oStream >> oTriangle.m_oColor;
            oStream >> oTriangle.m_oMaterials;
            break;

        default:
            break;
    }

    return oStream;
}

CRayStream &operator <<( CRayStream &oStream, CTriangle &oTriangle )
{
    oStream << TRIANGLE_VERSION;
    oStream << oTriangle.m_oCorner1;
    oStream << oTriangle.m_oCorner2;
    oStream << oTriangle.m_oCorner3;
    oStream << oTriangle.m_oSurfaceNormal;
    oStream << oTriangle.m_oNormal1;
    oStream << oTriangle.m_oNormal2;
    oStream << oTriangle.m_oNormal3;
    oStream << oTriangle.m_oColor;
    oStream << oTriangle.m_oMaterials;

    return oStream;
}


BOOL CTriangle::Intersect( CRay &oRay, CVector &oIntersect )
{
    FLT     nA;
    FLT     nB;
    FLT     nC;
    FLT     nD;
    FLT     nDenom;
    FLT     nNumer;
    FLT     nT;
    CVector oLine1, oLine2, oLine3;
    FLT     nAngle;
    CVector oPoint;

        // Here we develop the parametric equation for a plane
    nA = m_oSurfaceNormal.GetX();
    nB = m_oSurfaceNormal.GetY();
    nC = m_oSurfaceNormal.GetZ();
    nD = -( (m_oSurfaceNormal.GetX() * m_oCorner1.GetX() ) + 
            (m_oCorner1.GetY() * m_oSurfaceNormal.GetY() ) +
            (m_oCorner1.GetZ() * m_oSurfaceNormal.GetZ() ) );

        // The solution is given by t= -(ax+by+cz+d)/(ai+bj+ck)

    nNumer = - ( (nA * oRay.GetX()) + ( nB * oRay.GetY() ) + ( nC * oRay.GetZ() ) + nD );
    nDenom = ( nA * oRay.GetI()) + ( nB * oRay.GetJ() ) + ( nC * oRay.GetK() );

    if ( nDenom == 0.0 )
        return FALSE;

    nT = nNumer / nDenom;

    if ( ( nT > 0.0 ) && ( nT < oRay.GetT() ) )
    {
            // We know that we intersect the plane of the triangle, now we limit it to within
            // the boundaries of the triangle.
        oPoint.Set( (oRay.GetI() * nT) + oRay.GetX(), 
                    (oRay.GetJ() * nT) + oRay.GetY(),
                    (oRay.GetK() * nT) + oRay.GetZ() );
        nAngle = 0.0;
        oLine1 = m_oCorner1 - oPoint;
        oLine2 = m_oCorner2 - oPoint;
        oLine3 = m_oCorner3 - oPoint;
        oLine1.Normalize();
        oLine2.Normalize();
        oLine3.Normalize();

        nAngle = ACOS( oLine1.Dot( oLine2 ) );
        nAngle += ACOS( oLine2.Dot( oLine3 ) );
        nAngle += ACOS( oLine3.Dot( oLine1 ) );

        nAngle = ABS( nAngle - ( 2 * PI ) );

        if ( nAngle < 1e-6 )
        {
            oRay.SetT( nT );
            oIntersect = oPoint;
            return TRUE;
        }
    }

        // No intersection
    return FALSE;
}

CColor CTriangle::Shade( CLight *pLight, CVector &oIntersect ) // CArray <CLight *, CLight *> *pLight, CVector &oIntersect )
{
    CColor  oColor;
    return oColor;
}

CColor CTriangle::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
    CVector     oVector;
    CVector     oLine1, oLine2, oLine3;
    CVector     oSide;
    FLT         Mag1, Mag2, Mag3;
    CVector     oTemp1, oTemp2, oTemp3, oTemp4;

    oLine1 = oIntersect - m_oCorner1;
    Mag1 = oLine1.Magnitude();
    oTemp1 = m_oNormal1 * ( 1.0 / Mag1 );

    oLine2 = oIntersect - m_oCorner2;
    Mag2 = oLine2.Magnitude();
    oTemp2 = m_oNormal2 * ( 1.0 / Mag2 );

    oLine3 = oIntersect - m_oCorner3;
    Mag3 = oLine3.Magnitude();
    oTemp3 = m_oNormal3 * ( 1.0 / Mag3 );

    oTemp4 = oTemp1 + oTemp2;

    oVector = oTemp3 + oTemp4;

    oVector.Normalize();

    return BaseShade( oScene, oRay, oIntersect, oVector );
}

