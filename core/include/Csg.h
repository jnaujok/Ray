/*************************************************************************
*   CCsg                                                                 *
*                                                                        *
*   Constructive solid geometry: a solid made from others by union      *
*   (inside any of them), intersection (inside all of them) or          *
*   difference (inside the first and none of the rest). The shapes are  *
*   any closed objects: spheres, boxes, capped cylinders and cones,      *
*   meshes (which must be closed) and other csgs.                        *
*                                                                        *
*   Along a ray it finds every place each shape is crossed (asking the   *
*   shape again from just past each crossing), and so whether the ray    *
*   starts inside each: an odd number of crossings ahead means inside.   *
*   It then walks the crossings in order; the first where the solid's    *
*   inside changes is the hit. The normal there is the crossed shape's,  *
*   turned inside out for a subtracted shape.                            *
*                                                                        *
*   The csg shades with its own materials and bump map, as one object,   *
*   so glass made of parts is entered and left as one piece. With        *
*   childMaterials each surface is shaded with the materials of the      *
*   shape it comes from instead (a white die with black pips).           *
*                                                                        *
*   Like a mesh, it keeps what its last Intersect hit for Shade, so a    *
*   scene that traces on several threads gives each its own copy.        *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __CSG_H__
#define __CSG_H__

#include "Objects.h"

#include <vector>

class CCsg : public CObjects
{
    public:
        enum EOperation { CSG_UNION = 0, CSG_INTERSECTION = 1, CSG_DIFFERENCE = 2 };

    protected:
        int                     m_nOperation;
        std::vector<CObjects *> m_apParts;          // owned
        BOOL                    m_bChildMaterials;
        CColor                  m_oColor;

            // Worked out when first needed (Prepare).
        BOOL                    m_bPrepared;
        BOOL                    m_bHasBounds;
        CVector                 m_oMinimum;
        CVector                 m_oMaximum;
        FLT                     m_nEpsilon;         // a hair, in scene units, for the solid's size

            // What the last Intersect hit, for Shade, and the shape being
            // shaded (for its surface coordinates and materials).
        CObjects               *m_pHitLeaf;
        BOOL                    m_bHitFlip;
        CObjects               *m_pShadeLeaf;

        void                    Prepare( void );
        BOOL                    Inside( const std::vector<char> &abInside ) const;
            // The closest hit past nMinimum (scene units along the ray).
        BOOL                    Find( CRay &oRay, CVector &oIntersect, FLT nMinimum );

    public:
                                DYNAMIC_OBJECTS( CCsg );
                                CCsg( void );
        virtual                ~CCsg( void );

            /// Builds one in code: the operation, then the shapes (taking
            /// ownership; each must be closed).
        void                    SetOperation( int nOperation ) { m_nOperation = nOperation; m_bPrepared = FALSE; }
        void                    AddPart( CObjects *pPart ) { m_apParts.push_back( pPart ); m_bPrepared = FALSE; }
        void                    SetChildMaterials( BOOL bChildMaterials ) { m_bChildMaterials = bChildMaterials; }
        int                     GetOperation( void ) const { return m_nOperation; }
        int                     GetPartCount( void ) const { return (int)m_apParts.size(); }
        CObjects               *GetPart( int nIndex ) { return m_apParts[ (size_t)nIndex ]; }

        virtual BOOL            Intersect( CRay &oRay, CVector &oIntersect );
            // Ignoring itself (a shadow or reflected ray leaving its
            // surface), it skips only the surface the ray leaves from, so a
            // bowl shadows and reflects its own inside.
        virtual BOOL            Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore );
        virtual BOOL            GetBounds( CVector &oMinimum, CVector &oMaximum );
        virtual BOOL            SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV );
            // A part's own colors and texture space carry through, as its
            // surface coordinates do.
        virtual BOOL            SurfaceColor( CVector &oPoint, CColor &oColor );
        virtual CVector         TextureSpace( const CVector &oPoint ) const;
        virtual BOOL            SurfaceNormal( CVector &oPoint, CVector &oNormal );
        virtual CObjects       *HitPart( BOOL &bFlip ) { bFlip = m_bHitFlip; return m_pHitLeaf ? m_pHitLeaf : (CObjects *)this; }
        virtual BOOL            IsClosed( void ) { return TRUE; }
        virtual CColor          Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );
};

#endif // __CSG_H__
