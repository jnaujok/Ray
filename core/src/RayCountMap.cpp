/*************************************************************************
*   CRayCountMap                                                         *
*                                                                        *
*   Rays per pixel, for heat maps. See RayCountMap.h.                    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifdef RAY_GUI
#include "StdAfx.h"
#endif

#include "RayCountMap.h"

namespace
{
    std::vector<int>   *s_panCounts = nullptr;
    int                 s_nWidth = 0;
    int                 s_nHeight = 0;
}

void CRayCountMap::Enable( int nWidth, int nHeight )
{
    Disable();
    if ( nWidth > 0 && nHeight > 0 )
    {
        s_panCounts = new std::vector<int>( (size_t)nWidth * (size_t)nHeight, 0 );
        s_nWidth = nWidth;
        s_nHeight = nHeight;
    }
}

void CRayCountMap::Disable( void )
{
    delete s_panCounts;
    s_panCounts = nullptr;
    s_nWidth = s_nHeight = 0;
}

void CRayCountMap::Record( int nX, int nY, int nRays )
{
    if ( s_panCounts && nX >= 0 && nY >= 0 && nX < s_nWidth && nY < s_nHeight )
        ( *s_panCounts )[ (size_t)nY * (size_t)s_nWidth + (size_t)nX ] = nRays;
}

bool CRayCountMap::IsEnabled( void )
{
    return s_panCounts != nullptr;
}

int CRayCountMap::GetWidth( void )
{
    return s_nWidth;
}

int CRayCountMap::GetHeight( void )
{
    return s_nHeight;
}

const std::vector<int> &CRayCountMap::GetCounts( void )
{
    static const std::vector<int> s_anNone;

    return s_panCounts ? *s_panCounts : s_anNone;
}
