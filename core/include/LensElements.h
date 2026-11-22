/*************************************************************************
*   CLensElements                                                        *
*                                                                        *
*   A camera lens built as glass in the scene, from one of the built-in  *
*   prescriptions, with the rays it traces drawn through it: a lens      *
*   diagram rendered in 3D.                                              *
*                                                                        *
*   Each element (the glass between two surfaces) is a csg of the two    *
*   surfaces, spheres (a flat surface is a very large sphere), and a     *
*   capped cylinder at the element's clear aperture, made of a           *
*   transparent glass with the element's index and Abbe number. The      *
*   aperture stop is a thin dark ring opened to the f-number, and an     *
*   optional barrel surrounds the glass. With "cutaway" the half of      *
*   everything on the lens's -x side is cut off, so from that side the   *
*   cut faces show each element's cross-section.                         *
*                                                                        *
*   The ray fans are traced through the prescription with CLens's own    *
*   trace (CLens::TracePath), from far in front of the lens to the       *
*   sensor plane (focused at infinity): one fan for each field angle in  *
*   "fields" (fractions of the half-diagonal of a 36 x 24 mm frame, or  *
*   of the sensor width given), "rays" rays spread evenly over the part  *
*   of the front element the lens lets through, in the plane x = 0 (the  *
*   meridional plane). The on-axis fan is traced in red, green and blue  *
*   (the C, d and F lines), so its colors part near the focus; each      *
*   other fan in one color. Each ray is a thin tube (a sweep) with an    *
*   ambient-only material, so it shows bright and unlit, set just in     *
*   front of the cut face.                                               *
*                                                                        *
*   The lens is built in its own frame, in mm: the axis along +z (light  *
*   travels toward +z), the front vertex at z = 0, +y up. It is placed   *
*   in the scene scaled by "scale" (scene units per mm), turned by       *
*   "rotation" (degrees about x, then y, then z, as other shapes turn),  *
*   and moved to "position".                                             *
*                                                                        *
*   It is one object made of the generated parts; a scene file keeps    *
*   only its settings, and the parts are made again when it is read.    *
*   A ray that leaves a part may meet that part again (the far side of   *
*   an element) or any other part.                                       *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __LENSELEMENTS_H__
#define __LENSELEMENTS_H__

#include "Objects.h"
#include "Vector.h"

#include <string>
#include <vector>

class CLensElements : public CObjects
{
    protected:
        std::string             m_strPrescription;
        FLT                     m_nFNumber;         // 0: wide open
        FLT                     m_nScale;           // scene units per mm
        CVector                 m_oPosition;
        CVector                 m_oRotation;        // degrees
        BOOL                    m_bCutaway;
        BOOL                    m_bBarrel;
        BOOL                    m_bFans;
        int                     m_nRays;            // rays in each fan
        std::vector<FLT>        m_anFields;         // field fractions
        FLT                     m_nTubeRadius;      // mm
        FLT                     m_nLead;            // mm of ray in front of the lens; 0: half the lens's length
        FLT                     m_nSensorWidth;     // mm, for the field's size; 0: 36 x 24 mm

            // The generated parts, in order: elements, stop, barrel, rays.
        std::vector<CObjects *> m_apParts;
        int                     m_nElements;        // how many of the parts are glass
        int                     m_nRayCount;        // how many are ray tubes
        CVector                 m_oMinimum;
        CVector                 m_oMaximum;
        BOOL                    m_bHasBounds;

            // The part the last Intersect hit, and the one being shaded now
            // (whose own rays may meet it again).
        CObjects               *m_pHitPart;
        CObjects               *m_pShadePart;

        void                    Clear( void );
        CVector                 ToScene( FLT nX, FLT nY, FLT nZ ) const;
            // Builds the parts' JSON from the settings, then the parts.
        BOOL                    Build( std::string &strError );

    public:
                                DYNAMIC_OBJECTS( CLensElements );

                                CLensElements( void );
        virtual                ~CLensElements( void );

        virtual BOOL            Intersect( CRay &oRay, CVector &oIntersect );
        virtual BOOL            Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore );
        virtual BOOL            Occludes( CRay &oRay, CObjects *pIgnore );
        virtual BOOL            GetBounds( CVector &oMinimum, CVector &oMaximum );
        virtual BOOL            SurfaceNormal( CVector &oPoint, CVector &oNormal );
        virtual BOOL            SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV );
        virtual CObjects       *HitPart( BOOL &bFlip );
        virtual CColor          Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );

            /// The generated parts (for the editor's preview and tests).
        const std::vector<CObjects *> &GetParts( void ) const { return m_apParts; }
        int                     GetElementCount( void ) const { return m_nElements; }
        int                     GetRayCount( void ) const { return m_nRayCount; }

            /// The ray fans' paths, in scene coordinates, as they were built:
            /// each a list of points, the last on the sensor plane.
        const std::vector< std::vector<CVector> > &GetRayPaths( void ) const { return m_aaoRayPaths; }

    protected:
        std::vector< std::vector<CVector> > m_aaoRayPaths;
};

#endif // __LENSELEMENTS_H__
