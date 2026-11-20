/*************************************************************************
*   CSphere                                                              *
*                                                                        *
*       This class defines the behavior of the basic sphere. It handles  *
* all of the sphere intersection and shading information.                *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
    #include "resource.h"
    #include "SpherePropertiesDlg.h"
#else
    #include "Globals.h"
#endif

#include "Sphere.h"
#include "Light.h"
#include "SceneJson.h"

#include <math.h>

#ifdef RAY_GUI
#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

#define SPHERE_VERSION  ((DWORD)3)
#define VERSION_2       ((DWORD)2)
#define VERSION_1       ((DWORD)1)
                  
CSphere::CSphere( void )
{
    CMaterial   *pMaterial;

    m_nRadius = 0.0;

    pMaterial = new CDiffuseMaterial;
    m_oMaterials.Add( pMaterial, 1.0 );
    m_oColor.Set( 0.5, 0.5, 0.5 );
}

CSphere::CSphere( FLT nX, FLT nY, FLT nZ, FLT nRadius ) :
m_oCenter( nX, nY, nZ )
{
    m_nRadius = nRadius;
    m_oColor.Set( 0.5, 0.5, 0.5 );
}

CSphere::CSphere( CVector oCenter, FLT nRadius ) :
m_oCenter( oCenter )
{
    m_nRadius = nRadius;
    m_oColor.Set( 0.5, 0.5, 0.5 );
}

CSphere::~CSphere( void )
{
}


#ifdef RAY_GUI
void CSphere::ToTree( CSceneTreeView *pTree, HTREEITEM hUnder )
{
    HTREEITEM   hSphere;

    hSphere = pTree->AddObject( this, 8, "Sphere", hUnder );
    CObjects::MatToTree( pTree, hSphere );
}

void CSphere::SetProperties( void )
{
    CSpherePropertiesDlg    oProp( &m_oCenter, &m_nRadius );
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

DYNAMIC_IMPLEMENTS( CSphere, "sphere" );


BOOL CSphere::Intersect( CRay &oRay, CVector &oIntersect )
{
    BOOL    bFound = FALSE;
    FLT     nA;
    FLT     nB;
    FLT     nC;
    FLT     nT;
    FLT     nT1;
    FLT     nT2;
    FLT     nBSquared;
    FLT     n4AC;
    FLT     nSqrt;
    FLT     n2A;
    FLT     nDet;

    // TO DO: Speed-ups -- For instance, since the rays are normalized, nA is _always_ 1.

        // A = i^2 + j^2 + k^2
    nA = ( oRay.GetI() * oRay.GetI() ) + (oRay.GetJ() * oRay.GetJ() ) + (oRay.GetK() * oRay.GetK() );
        // B = 2i(x-l) + 2j(y-m) + 2k(z-n)
    nB = (( 2.0 * oRay.GetI() ) * (oRay.GetX() - m_oCenter.GetX() )) +
         (( 2.0 * oRay.GetJ() ) * (oRay.GetY() - m_oCenter.GetY() )) +
         (( 2.0 * oRay.GetK() ) * (oRay.GetZ() - m_oCenter.GetZ() ));
        // C = l^2 + m^2 + n^2 + x^2 + y^2 + z^2 + 2(-lx - my -nz) - r^2
    nC = ( m_oCenter.GetX() * m_oCenter.GetX() ) +
         ( m_oCenter.GetY() * m_oCenter.GetY() ) +
         ( m_oCenter.GetZ() * m_oCenter.GetZ() ) +
         ( oRay.GetX() * oRay.GetX() ) +
         ( oRay.GetY() * oRay.GetY() ) +
         ( oRay.GetZ() * oRay.GetZ() ) +
         ( 2.0 * ( ( -m_oCenter.GetX() * oRay.GetX() ) +
                   ( -m_oCenter.GetY() * oRay.GetY() ) +
                   ( -m_oCenter.GetZ() * oRay.GetZ() ) ) ) -
         ( m_nRadius * m_nRadius );
    nBSquared = nB * nB;
    n4AC = 4.0 * nA * nC;
    n2A = 2.0 * nA;
    nDet = nBSquared - n4AC;

        // A negative determinant means all intersections are imaginary (i.e. The line never intersects)
    if ( nDet < 0.0 )
        return FALSE;

        // A zero determinant means the intersection is tangential, so we just say it hits.
        // The truth is, this code will probably never be hit, since the solution is unlikely to
        // _EVER_ be exactly zero.
    if ( nDet == 0.0 )
    {
        nT = -nB / n2A;
        if ( ( nT > 0.0 ) && ( nT < oRay.GetT() ) )
        {
            oRay.SetT( nT );
            oIntersect.Set( (oRay.GetI() * nT) + oRay.GetX(), 
                            (oRay.GetJ() * nT) + oRay.GetY(),
                            (oRay.GetK() * nT) + oRay.GetZ() );
            return TRUE;
        }
    }

        // Determinant is positive, we have to find the nearer intersection
    nSqrt = SQRT( nBSquared - n4AC );

    nT1 = ( -nB - nSqrt ) / n2A;
    nT2 = ( -nB + nSqrt ) / n2A;

    if ( ( nT1 > 0.0 ) && ( nT1 < nT2 ) )
        nT = nT1;
    else if ( nT2 > 0.0 )
        nT = nT2;
    else if ( nT1 > 0.0 )
        nT = nT1;
    else
        return FALSE;

        // If the intersection is the closest, return TRUE;
    if ( ( nT > 0.0 ) && ( nT < oRay.GetT() ) )
    {
        if ( ( ( nT1 > 0.0 ) && ( nT2 < 0.0 ) ) || ( (nT2 > 0.0 ) && (nT1 < 0.0 ) ) )
            oIntersect.Inside( TRUE );
        else
            oIntersect.Inside( FALSE );
        oRay.SetT( nT );
        oIntersect.Set( (oRay.GetI() * nT) + oRay.GetX(), 
                        (oRay.GetJ() * nT) + oRay.GetY(),
                        (oRay.GetK() * nT) + oRay.GetZ() );
        return TRUE;
    }

        // No intersection
    return FALSE;
}

CColor CSphere::Shade( CLight *pLight, CVector &oIntersect ) // CArray <CLight *, CLight *> *pLight, CVector &oIntersect )
{
    CColor  oColor;
    return oColor;
}

CColor CSphere::Shade( CScene &oScene, CRay &oRay, CVector &oIntersect )
{
    CVector     oVector;

    oVector = oIntersect - m_oCenter;

    oVector.Normalize();

    return BaseShade( oScene, oRay, oIntersect, oVector );
}

void CSphere::ToJson( IASLib::CObjectNode &oNode )
{
    RayJson::PutVector( oNode, "center", m_oCenter );
    RayJson::PutNumber( oNode, "radius", m_nRadius );
    RayJson::PutColor( oNode, "color", m_oColor );
    m_oMaterials.ToJson( *oNode.putArray( "materials" ) );
    BumpToJson( oNode );
}

BOOL CSphere::FromJson( const IASLib::CJsonNode &oNode )
{
    static const char *const astrFields[] = { "center", "radius", "color", "materials", "bump", NULL };

    return RayJson::OnlyFields( oNode, astrFields ) &&
           RayJson::GetVector( oNode, "center", m_oCenter, TRUE ) &&
           RayJson::GetNumber( oNode, "radius", m_nRadius, TRUE ) &&
           RayJson::GetColor( oNode, "color", m_oColor, FALSE ) &&
           m_oMaterials.FromJson( oNode, "materials" ) &&
           BumpFromJson( oNode );
}

BOOL CSphere::SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV )
{
    CVector oFromCenter = oPoint - m_oCenter;

    oFromCenter.Normalize();

    FLT nY = oFromCenter.GetY();

    if ( nY > 1.0 )
        nY = 1.0;
    else if ( nY < -1.0 )
        nY = -1.0;
    nU = 0.5 + ( (FLT)atan2( (double)oFromCenter.GetZ(), (double)oFromCenter.GetX() ) / ( 2.0 * PI ) );
    nV = 0.5 + ( ASIN( nY ) / PI );
    return TRUE;
}

    // A box a little larger than [anLow, anHigh], so hits a hair outside the
    // exact shape (as some tests allow) are inside it.
static void PadBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ], CVector &oMinimum, CVector &oMaximum )
{
    FLT nMargin = 1e-6 * ( 1.0 + ( anHigh[ 0 ] - anLow[ 0 ] ) + ( anHigh[ 1 ] - anLow[ 1 ] ) + ( anHigh[ 2 ] - anLow[ 2 ] ) );

    oMinimum.Set( anLow[ 0 ] - nMargin, anLow[ 1 ] - nMargin, anLow[ 2 ] - nMargin );
    oMaximum.Set( anHigh[ 0 ] + nMargin, anHigh[ 1 ] + nMargin, anHigh[ 2 ] + nMargin );
}

BOOL CSphere::GetBounds( CVector &oMinimum, CVector &oMaximum )
{
    FLT nRadius = ABS( m_nRadius );
    FLT anLow[ 3 ] = { m_oCenter.GetX() - nRadius, m_oCenter.GetY() - nRadius, m_oCenter.GetZ() - nRadius };
    FLT anHigh[ 3 ] = { m_oCenter.GetX() + nRadius, m_oCenter.GetY() + nRadius, m_oCenter.GetZ() + nRadius };

    PadBounds( anLow, anHigh, oMinimum, oMaximum );
    return TRUE;
}
