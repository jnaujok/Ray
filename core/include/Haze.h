/*************************************************************************
*   CHaze                                                                *
*                                                                        *
*   An atmosphere the scene sits in: haze, fog or ground mist. A ray     *
*   that travels through it is blended toward the haze's "color" by how  *
*   much of its light gets through, the transmittance                    *
*                                                                        *
*       T = exp( - integral of the density along the ray )               *
*                                                                        *
*   so what is far away fades into the haze, and a ray that leaves the   *
*   scene (the sky) is hazed over RAY_FAR_AWAY. Light on its way from a  *
*   light to a surface is dimmed by the same T along its path.           *
*                                                                        *
*   "density" is how thick it is (per scene unit) at "baseHeight". With  *
*   "heightFalloff" 0 (the default) it is the same everywhere; with a    *
*   falloff it thins with height, by e for every 1 / heightFalloff units *
*   above baseHeight (and thickens below it), like mist lying on the     *
*   ground. Up is +y. The integral has a closed form either way, so T    *
*   is exact and costs nothing to trace.                                 *
*                                                                        *
*   Sunlight (a directional light) comes from outside the scene: in      *
*   haze that is the same everywhere its path would never end, so it is  *
*   not dimmed then; with a falloff it is dimmed over its whole path up  *
*   and out of the haze.                                                 *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __HAZE_H__
#define __HAZE_H__

#include "Globals.h"
#include "Color.h"
#include "Vector.h"

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

class CHaze
{
    protected:
        CColor      m_oColor;
        FLT         m_nDensity;
        FLT         m_nHeightFalloff;
        FLT         m_nBaseHeight;

    public:
                    CHaze( void );

        const CColor &GetColor( void ) const { return m_oColor; }
        FLT         GetDensity( void ) const { return m_nDensity; }
        FLT         GetHeightFalloff( void ) const { return m_nHeightFalloff; }
        FLT         GetBaseHeight( void ) const { return m_nBaseHeight; }

            /// The integral of the density from oOrigin along the unit
            /// vector oDirection for nLength scene units.
        FLT         OpticalDepth( const CVector &oOrigin, const CVector &oDirection, FLT nLength ) const;
            /// How much light gets through that path, 0 to 1.
        FLT         Transmittance( const CVector &oOrigin, const CVector &oDirection, FLT nLength ) const;
            /// oColor seen through haze that lets nTransmittance through.
        CColor      Through( const CColor &oColor, FLT nTransmittance ) const;

        void        ToJson( IASLib::CObjectNode &oNode ) const;
        BOOL        FromJson( const IASLib::CJsonNode &oNode );
};

#endif // __HAZE_H__
