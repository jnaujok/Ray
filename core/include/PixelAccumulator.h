/*************************************************************************
*   CPixelAccumulator                                                    *
*                                                                        *
*   Weighted averages of samples for every pixel of an image, for        *
*   samplers that trace several rays a pixel. Each sample adds its color *
*   times a whole-number weight; when a pixel's weights reach the total  *
*   it expects, its average goes to the pixel writer and its sum is      *
*   cleared.                                                             *
*                                                                        *
*   Colors are summed unclipped (from 0 up to a large cap), so the color *
*   model sees each pixel's true brightness, and in fixed point, so the  *
*   result doesn't depend on the order samples come back in: threads and *
*   networked engines give the same image.                               *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __PIXELACCUMULATOR_H__
#define __PIXELACCUMULATOR_H__

#include "Globals.h"

#include <cstdint>
#include <vector>

class CColor;
class CPixelWriter;

class CPixelAccumulator
{
    protected:
        struct SPixelSum
        {
            uint64_t            nRed;
            uint64_t            nGreen;
            uint64_t            nBlue;
            uint32_t            nWeight;
        };

        int                     m_nLeft;            // the part of the image held
        int                     m_nTop;
        int                     m_nWidth;
        int                     m_nHeight;
        uint32_t                m_nTotalWeight;
        std::vector<SPixelSum>  m_aoSums;

    public:
                                CPixelAccumulator( void );

            /// Starts the part of an image nWidth by nHeight pixels from
            /// (nLeft, nTop); each pixel is done when its samples' weights add
            /// up to nTotalWeight.
        void                    Begin( int nLeft, int nTop, int nWidth, int nHeight, uint32_t nTotalWeight );
            /// Adds a sample to pixel (nX, nY), in image coordinates. Pixels
            /// outside the part held are ignored, so a sample shared with
            /// pixels beyond it can be added to all its pixels without
            /// checking.
        void                    Add( CPixelWriter &oWriter, int nX, int nY, const CColor &oColor, uint32_t nWeight );
            /// Frees the sums.
        void                    End( void );
};

#endif // __PIXELACCUMULATOR_H__
