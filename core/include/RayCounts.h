/*************************************************************************
*   CRayCounts                                                           *
*                                                                        *
*   How many rays of each kind a render cast into the scene: camera      *
*   rays, shadow rays (by the kind of light they test), bounce rays for  *
*   indirect light, reflected and refracted rays (rough and dispersed    *
*   ones included), and anything else. A ray stopped by the depth limit  *
*   or the depth threshold before it is traced isn't counted.            *
*                                                                        *
*   Each task counts into its own copy of the scene (no locks or atomics *
*   on the way), and sends a tile's counts with its pixels; the master   *
*   adds them up. Every ray is the same in every run and every task, so  *
*   threads and networked engines report the same totals.                *
*                                                                        *
*   A real lens's recasts of rays it blocks happen inside the lens, not  *
*   in the scene, so they aren't counted here.                           *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __RAYCOUNTS_H__
#define __RAYCOUNTS_H__

#include <cstdint>
#include <cstring>

class CRayCounts
{
    public:
        enum EKind
        {
            CAMERA = 0,
            SHADOW_POINT,       // shadow rays toward point lights
            SHADOW_SPOT,        // toward spot lights (a spot with a radius too)
            SHADOW_AREA,        // toward sphere and rectangle lights
            SHADOW_SUN,         // toward directional lights
            SHADOW_SKY,         // toward the environment, for sky lighting
            INDIRECT,           // gathering bounced light
            REFLECTION,
            REFRACTION,
            OTHER,
            KINDS
        };

        uint64_t    anCounts[ KINDS ];

                    CRayCounts( void ) { Clear(); }

        void        Clear( void ) { memset( anCounts, 0, sizeof( anCounts ) ); }
        void        Count( int nKind ) { anCounts[ nKind ]++; }

        void        Add( const CRayCounts &oOther )
        {
            for ( int n = 0; n < KINDS; n++ )
                anCounts[ n ] += oOther.anCounts[ n ];
        }

            /// What was counted since oBefore.
        CRayCounts  Since( const CRayCounts &oBefore ) const
        {
            CRayCounts oOut;

            for ( int n = 0; n < KINDS; n++ )
                oOut.anCounts[ n ] = anCounts[ n ] - oBefore.anCounts[ n ];
            return oOut;
        }

        uint64_t    Total( void ) const
        {
            uint64_t nTotal = 0;

            for ( int n = 0; n < KINDS; n++ )
                nTotal += anCounts[ n ];
            return nTotal;
        }

        uint64_t    Shadows( void ) const
        {
            return anCounts[ SHADOW_POINT ] + anCounts[ SHADOW_SPOT ] + anCounts[ SHADOW_AREA ] + anCounts[ SHADOW_SUN ] +
                   anCounts[ SHADOW_SKY ];
        }

        bool        operator==( const CRayCounts &oOther ) const { return memcmp( anCounts, oOther.anCounts, sizeof( anCounts ) ) == 0; }

            /// The kind's name, as the summary and its JSON give it.
        static const char *Name( int nKind )
        {
            static const char *const astrNames[ KINDS ] =
            {
                "camera", "shadowPoint", "shadowSpot", "shadowArea", "shadowSun", "shadowSky", "indirect", "reflection",
                "refraction", "other"
            };

            return ( nKind >= 0 && nKind < KINDS ) ? astrNames[ nKind ] : "?";
        }
};

#endif // __RAYCOUNTS_H__
