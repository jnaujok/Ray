/*************************************************************************
*   CRayCountMap                                                         *
*                                                                        *
*   How many camera rays each pixel of an image took, for samplers that  *
*   decide that per pixel (the adaptive sampler). raythrower's           *
*   --ray-count turns it on before tracing with threads and draws it as  *
*   a heat map afterwards. Each pixel is written by the one task that    *
*   traces its tile, so tasks never write the same entry.                *
*                                                                        *
*   Off unless enabled, so recording costs a test of one pointer.        *
*   Networked engines don't record (their counts never cross the wire).  *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __RAYCOUNTMAP_H__
#define __RAYCOUNTMAP_H__

#include <cstddef>
#include <vector>

class CRayCountMap
{
    public:
            /// Starts recording an image this size (every count 0).
        static void                 Enable( int nWidth, int nHeight );
            /// Stops recording and frees the counts.
        static void                 Disable( void );
            /// Notes that pixel (nX, nY) took nRays rays, if recording.
        static void                 Record( int nX, int nY, int nRays );
        static bool                 IsEnabled( void );
        static int                  GetWidth( void );
        static int                  GetHeight( void );
            /// The counts in reading order (0: the sampler didn't say).
        static const std::vector<int> &GetCounts( void );
};

#endif // __RAYCOUNTMAP_H__
