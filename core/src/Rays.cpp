/*************************************************************************
*   CRay                                                                 *
*                                                                        *
*       This class defines a single ray to be cast into the scene. A ray *
* consists of a direction (the i, j, k values) and a starting point (the *
* x, y, z values). It also has the time of closest intersection stored   *
* in it to make the depth queueing of rays much simpler since we are     *
* only ever worried about the closest intersection with the ray.         *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
    #include "StdAfx.h"
#else
    #include "Globals.h"
#endif

#include "Objects.h"
#include "Transform.h"
#include "Rays.h"
#include "Camera.h"
#include "Scene.h"

#if defined(RAY_GUI) && defined(_DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define RAY_VERSION     ((DWORD)1)

CRay::CRay( void )
{
    m_nI = 0.0;
    m_nJ = 0.0;
    m_nK = 0.0;
    m_nX = 0.0;
    m_nY = 0.0;
    m_nZ = 0.0;
    m_nT = 1e100;       // effective infinity
    m_nInfluence = 0.0;
    m_nPixelX = 0;
    m_nPixelY = 0;
    m_nDepth = 0;
}

CRay::CRay( const CRay &oRay )
{
    m_nI = oRay.m_nI;
    m_nJ = oRay.m_nJ;
    m_nK = oRay.m_nK;
    m_nX = oRay.m_nX;
    m_nY = oRay.m_nY;
    m_nZ = oRay.m_nZ;
    m_nT = oRay.m_nT;
    m_nInfluence = oRay.m_nInfluence;
    m_nPixelX = oRay.m_nPixelX;
    m_nPixelY = oRay.m_nPixelY;
    m_nDepth = oRay.m_nDepth;
}

CRay::~CRay( void )
{
}

CRay &CRay::operator =( const CRay &oRay )
{
    m_nI = oRay.m_nI;
    m_nJ = oRay.m_nJ;
    m_nK = oRay.m_nK;
    m_nX = oRay.m_nX;
    m_nY = oRay.m_nY;
    m_nZ = oRay.m_nZ;
    m_nT = oRay.m_nT;
    m_nInfluence = oRay.m_nInfluence;
    m_nPixelX = oRay.m_nPixelX;
    m_nPixelY = oRay.m_nPixelY;
    m_nDepth = oRay.m_nDepth;

    return *this;
}

void CRay::CreateRay( CVector &oVector, CVector &oOrigin, FLT nInfluence, int nX, int nY )
{
    CVector     oNormal( oVector );

    oNormal.Normalize();

    m_nX = oOrigin.GetX();
    m_nY = oOrigin.GetY();
    m_nZ = oOrigin.GetZ();
    m_nI = oNormal.GetX();
    m_nJ = oNormal.GetY();
    m_nK = oNormal.GetZ();
    m_nT = oVector.Magnitude();
    m_nInfluence = nInfluence;
    m_nPixelX = nX;
    m_nPixelY = nY;
    m_nDepth = 0;
}

    // The primary ray from the camera through the center of pixel (nX, nY),
    // where (0, 0) is the top left corner of the image.
void CRay::CreateRay( CScene &oScene, int nX, int nY, FLT nInfluence )
{
    CreateRayAt( oScene, nX + 0.5, nY + 0.5, nX, nY, nInfluence );
}

    // The primary ray from the camera through the point (nImageX, nImageY) on
    // the image, measured in pixels from its top left corner: (0, 0) is that
    // corner and (width, height) the bottom right one. The ray is labeled
    // with (nTagX, nTagY), which comes back with its color.
void CRay::CreateRayAt( CScene &oScene, FLT nImageX, FLT nImageY, int nTagX, int nTagY, FLT nInfluence )
{
    CCamera        &oCamera = oScene.GetCamera();
    CSettings      &oSettings = oScene.GetSettings();
    FLT             nWidth = (FLT)oSettings.GetImageWidth();
    FLT             nHeight = (FLT)oSettings.GetImageHeight();
    CVector         oStartPos = oCamera.GetPosition();
    CVector         oDirection;

    if ( ( nWidth <= 0.0 ) || ( nHeight <= 0.0 ) )
        nWidth = nHeight = 1.0;

    oDirection = oCamera.RayDirection( ( ( nImageX * 2.0 ) / nWidth ) - 1.0,
                                       1.0 - ( ( nImageY * 2.0 ) / nHeight ),
                                       nWidth / nHeight );

    m_nX = oStartPos.GetX();
    m_nY = oStartPos.GetY();
    m_nZ = oStartPos.GetZ();
    m_nI = oDirection.GetX();
    m_nJ = oDirection.GetY();
    m_nK = oDirection.GetZ();

    m_nT = 1e100;

    m_nInfluence = nInfluence;
    m_nPixelX = nTagX;
    m_nPixelY = nTagY;
    m_nDepth = 0;
}

void CRay::SetT( FLT nNewT )
{
    m_nT = nNewT;
}

CRayStream  &operator >>( CRayStream &oStream, CRay &oRay )
{
    DWORD       dwVersion;

    oStream >> dwVersion;

    switch ( dwVersion )
    {
        case RAY_VERSION:
            oStream >> oRay.m_nI;
            oStream >> oRay.m_nJ;
            oStream >> oRay.m_nK;
            oStream >> oRay.m_nX;
            oStream >> oRay.m_nY;
            oStream >> oRay.m_nZ;
            oStream >> oRay.m_nT;
            oStream >> oRay.m_nInfluence;
            oStream >> oRay.m_nPixelX;
            oStream >> oRay.m_nPixelY;
            break;

        default:
            break;
    }
    return oStream;
}


CRayStream  &operator <<( CRayStream &oStream, CRay &oRay )
{
    oStream << RAY_VERSION;
    oStream << oRay.m_nI;
    oStream << oRay.m_nJ;
    oStream << oRay.m_nK;
    oStream << oRay.m_nX;
    oStream << oRay.m_nY;
    oStream << oRay.m_nZ;
    oStream << oRay.m_nT;
    oStream << oRay.m_nInfluence;
    oStream << oRay.m_nPixelX;
    oStream << oRay.m_nPixelY;
    return oStream;
}

