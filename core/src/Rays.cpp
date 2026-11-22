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

    #include "Globals.h"

#include "Objects.h"
#include "Transform.h"
#include "Rays.h"
#include "Camera.h"
#include "Scene.h"
#include "Sampling.h"

#include <string.h>



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
    m_nWeight = 1.0;
    m_nPixelX = 0;
    m_nPixelY = 0;
    m_nDepth = 0;
    m_nImageX = 0.0;
    m_nImageY = 0.0;
    m_pInside = NULL;
    m_nSampleU = 0.5;
    m_nSampleV = 0.5;
    m_nSampleKey = 0;
    m_nSampleIndex = 0;
    m_nSampleCount = 0;
    m_nChannel = 0;
    m_nBounce = 0;
    m_bGather = FALSE;
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
    m_nWeight = oRay.m_nWeight;
    m_nPixelX = oRay.m_nPixelX;
    m_nPixelY = oRay.m_nPixelY;
    m_nDepth = oRay.m_nDepth;
    m_nImageX = oRay.m_nImageX;
    m_nImageY = oRay.m_nImageY;
    m_pInside = oRay.m_pInside;
    m_nSampleU = oRay.m_nSampleU;
    m_nSampleV = oRay.m_nSampleV;
    m_nSampleKey = oRay.m_nSampleKey;
    m_nSampleIndex = oRay.m_nSampleIndex;
    m_nSampleCount = oRay.m_nSampleCount;
    m_nChannel = oRay.m_nChannel;
    m_nBounce = oRay.m_nBounce;
    m_bGather = oRay.m_bGather;
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
    m_nWeight = oRay.m_nWeight;
    m_nPixelX = oRay.m_nPixelX;
    m_nPixelY = oRay.m_nPixelY;
    m_nDepth = oRay.m_nDepth;
    m_nImageX = oRay.m_nImageX;
    m_nImageY = oRay.m_nImageY;
    m_pInside = oRay.m_pInside;
    m_nSampleU = oRay.m_nSampleU;
    m_nSampleV = oRay.m_nSampleV;
    m_nSampleKey = oRay.m_nSampleKey;
    m_nSampleIndex = oRay.m_nSampleIndex;
    m_nSampleCount = oRay.m_nSampleCount;
    m_nChannel = oRay.m_nChannel;
    m_nBounce = oRay.m_nBounce;
    m_bGather = oRay.m_bGather;

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
    m_nWeight = 1.0;
    m_nPixelX = nX;
    m_nPixelY = nY;
    m_nDepth = 0;
    m_pInside = NULL;
    m_nSampleU = 0.5;
    m_nSampleV = 0.5;
    m_nSampleKey = 0;
    m_nSampleIndex = 0;
    m_nSampleCount = 0;
    m_nChannel = 0;
    m_nBounce = 0;
    m_bGather = FALSE;
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
    // with (nTagX, nTagY), which comes back with its color. nChannel picks
    // the wavelength a chromatic lens traces it at (CLens::EChannel).
void CRay::CreateRayAt( CScene &oScene, FLT nImageX, FLT nImageY, int nTagX, int nTagY, FLT nInfluence, int nChannel )
{
    CCamera        &oCamera = oScene.GetCamera();
    CSettings      &oSettings = oScene.GetSettings();
    FLT             nWidth = (FLT)oSettings.GetImageWidth();
    FLT             nHeight = (FLT)oSettings.GetImageHeight();
    CVector         oStartPos;
    CVector         oDirection;

    if ( ( nWidth <= 0.0 ) || ( nHeight <= 0.0 ) )
        nWidth = nHeight = 1.0;

        // A ray the camera's lens blocks gets no influence, so it traces as
        // black.
    m_nWeight = 1.0;
    if ( ! oCamera.PrimaryRay( ( ( nImageX * 2.0 ) / nWidth ) - 1.0, 1.0 - ( ( nImageY * 2.0 ) / nHeight ),
                               nWidth / nHeight, nImageX, nImageY, oStartPos, oDirection, nChannel, &m_nWeight ) )
        nInfluence = 0.0;
    m_nImageX = nImageX;
    m_nImageY = nImageY;

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
    m_pInside = NULL;

        // The sample point is where the ray falls in its pixel; the key is
        // the pixel's, so every ray of a pixel is shifted alike.
    FLT nPixelX = (FLT)floor( (double)nImageX );
    FLT nPixelY = (FLT)floor( (double)nImageY );

    m_nSampleU = nImageX - nPixelX;
    m_nSampleV = nImageY - nPixelY;
    m_nSampleKey = RaySampling::Mix( RaySampling::Mix( (uint64_t)(int64_t)nPixelX ) ^ (uint64_t)(int64_t)nPixelY );
    m_nSampleIndex = 0;
    m_nSampleCount = 0;
    m_nChannel = nChannel;
    m_nBounce = 0;
    m_bGather = FALSE;
}

void CRay::Set( FLT nX, FLT nY, FLT nZ, FLT nI, FLT nJ, FLT nK, FLT nT, FLT nInfluence, int nPixelX, int nPixelY )
{
    m_nX = nX;
    m_nY = nY;
    m_nZ = nZ;
    m_nI = nI;
    m_nJ = nJ;
    m_nK = nK;
    m_nT = nT;
    m_nInfluence = nInfluence;
    m_nWeight = 1.0;
    m_nPixelX = nPixelX;
    m_nPixelY = nPixelY;
    m_nDepth = 0;
    m_pInside = NULL;
    m_nSampleU = 0.5;
    m_nSampleV = 0.5;
    m_nSampleKey = 0;
    m_nSampleIndex = 0;
    m_nSampleCount = 0;
    m_nChannel = 0;
    m_nBounce = 0;
    m_bGather = FALSE;
}

void CRay::SetT( FLT nNewT )
{
    m_nT = nNewT;
}

void CRay::InheritSample( const CRay &oParent, uint64_t nBranch )
{
        // The new ray's point is a hash of its parent's, not the same point:
        // the same one (only shifted) would tie each bounce's choice to the
        // one before in a way fixed for the pixel, and a pixel's rough glass
        // would blur by its own kernel, which more rays never smooth out.
        // Hashed, it is independent of the parent's (and still the same on
        // every run, from the parent's exact bits).
    uint64_t    nBitsU;
    uint64_t    nBitsV;
    double      dU = (double)oParent.m_nSampleU;
    double      dV = (double)oParent.m_nSampleV;

    memcpy( &nBitsU, &dU, sizeof( nBitsU ) );
    memcpy( &nBitsV, &dV, sizeof( nBitsV ) );
    m_nSampleKey = RaySampling::Mix( oParent.m_nSampleKey ^ RaySampling::Mix( nBranch + ( (uint64_t)oParent.m_nDepth << 40 ) ) );

    uint64_t    nHash = RaySampling::Mix( RaySampling::Mix( m_nSampleKey ^ nBitsU ) ^ nBitsV );

    m_nSampleU = RaySampling::Unit( nHash );
    m_nSampleV = RaySampling::Unit( RaySampling::Mix( nHash ) );
        // A ray made from another keeps its wavelength, and which of its
        // pixel's samples it came from.
    m_nChannel = oParent.m_nChannel;
    m_nBounce = oParent.m_nBounce;
    m_bGather = FALSE;
    m_nSampleIndex = oParent.m_nSampleIndex;
    m_nSampleCount = oParent.m_nSampleCount;
}

void CRay::SamplePoint( uint64_t nPurpose, FLT &nU, FLT &nV ) const
{
    uint64_t    nHash = RaySampling::Mix( m_nSampleKey ^ RaySampling::Mix( nPurpose + ( (uint64_t)m_nDepth << 40 ) ) );
    uint64_t    nTurn = RaySampling::Mix( RaySampling::Mix( nHash ) );
    FLT         nShiftedU = RaySampling::Wrap( m_nSampleU + RaySampling::Unit( nHash ) );
    FLT         nShiftedV = RaySampling::Wrap( m_nSampleV + RaySampling::Unit( RaySampling::Mix( nHash ) ) );

        // Turning and mirroring the square too, differently for each
        // purpose, keeps one use's points from lining up with another's.
    if ( nTurn & 1 )
        nShiftedU = ( nShiftedU > 0.0 ) ? 1.0 - nShiftedU : 0.0;
    if ( nTurn & 2 )
        nShiftedV = ( nShiftedV > 0.0 ) ? 1.0 - nShiftedV : 0.0;
    if ( nTurn & 4 )
    {
        nU = nShiftedV;
        nV = nShiftedU;
    }
    else
    {
        nU = nShiftedU;
        nV = nShiftedV;
    }
}

void CRay::SetSampleIndex( int nIndex, int nCount )
{
    if ( nCount > 0 && nIndex >= 0 && nIndex < nCount )
    {
        m_nSampleIndex = (uint32_t)nIndex;
        m_nSampleCount = (uint32_t)nCount;
    }
    else
    {
        m_nSampleIndex = 0;
        m_nSampleCount = 0;
    }
}

FLT CRay::WavelengthOffset( void ) const
{
        // The pixel's own shuffle: its key (not the ray's depth, so a
        // primary ray and the rays it makes agree on a sample's stratum).
    uint64_t    nHash = RaySampling::Mix( m_nSampleKey ^ RaySampling::Mix( (uint64_t)RaySampling::PURPOSE_WAVELENGTH ) );

    if ( m_nSampleCount > 1 )
    {
        uint32_t    nStratum = RaySampling::Permute( m_nSampleIndex, m_nSampleCount, (uint32_t)( nHash >> 32 ) );
        FLT         nJitter = RaySampling::Unit( RaySampling::Mix( nHash ^ RaySampling::Mix( (uint64_t)m_nSampleIndex + 1 ) ) );

        return ( (FLT)nStratum + nJitter ) / (FLT)m_nSampleCount;
    }

    uint64_t    nBitsU;
    uint64_t    nBitsV;
    double      dU = (double)m_nSampleU;
    double      dV = (double)m_nSampleV;

    memcpy( &nBitsU, &dU, sizeof( nBitsU ) );
    memcpy( &nBitsV, &dV, sizeof( nBitsV ) );
    return RaySampling::Unit( RaySampling::Mix( RaySampling::Mix( nHash ^ nBitsU ) ^ nBitsV ) );
}
