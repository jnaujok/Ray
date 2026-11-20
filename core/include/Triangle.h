/*************************************************************************
*   CTriangle                                                            *
*                                                                        *
*       This class defines the behavior of the basic triangle. It handles*
* all of the triangle intersection information.                          *
*                                                                        *
*       The surface normal comes from the corners, by the edge traversal *
* (V2 - V1) x (V3 - V2), so the order of the corners decides which side  *
* is the front. It is the rule OBJ and most other model formats are made *
* for, so their faces come out facing outward. A single-sided triangle   *
* can only be hit from the front; a double-sided one from either side,   *
* and it is shaded with the normal turned toward the viewer.             *
*                                                                        *
*       A flat triangle is shaded with its surface normal everywhere. A  *
* smooth one blends normals for its corners by barycentric weights, so   *
* neighboring faces meet without a seam. The corner normals come from a  *
* mesh (the sum of the normals of the faces around each vertex) or from  *
* the scene file; otherwise they are the surface normal.                 *
*                                                                        *
* Copyright (c) 1997, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __TRIANGLE_H__
#define __TRIANGLE_H__

#include "Objects.h"

class CTriangle : public CObjects
{
    protected:
        CVector             m_oCorner1;
        CVector             m_oCorner2;
        CVector             m_oCorner3;

        CVector             m_oSurfaceNormal;

            // The corner normals a smooth triangle blends.
        CVector             m_oNormal1;
        CVector             m_oNormal2;
        CVector             m_oNormal3;

        BOOL                m_bDoubleSided;
        BOOL                m_bSmooth;
        BOOL                m_bGivenNormals;    // the corner normals came from the scene file
        FLT                 m_anUVs[ 6 ];       // the corners' surface coordinates (u, v), if m_bHasUVs
        BOOL                m_bHasUVs;

            // The barycentric weights of a point on the triangle; FALSE if
            // the triangle has no area.
        BOOL                Weights( CVector &oPoint, FLT &nWeight1, FLT &nWeight2, FLT &nWeight3 );

        CColor              m_oColor;

            // Works out the surface normal from the corners, and makes it the
            // corner normals too.
        void                CalcNormal( void );

    public:
                            DYNAMIC_OBJECTS( CTriangle );

                            CTriangle( void );
            // A face of a mesh: it shares the mesh's materials (pMaterials)
            // without owning them.
                            CTriangle( const CVector &oCorner1, const CVector &oCorner2, const CVector &oCorner3,
                                       BOOL bDoubleSided, BOOL bSmooth, CMaterialList *pMaterials );
        virtual            ~CTriangle( void );

        CVector            &GetSurfaceNormal( void ) { return m_oSurfaceNormal; }
        CVector            &GetCorner( int nIndex ) { return ( nIndex == 0 ) ? m_oCorner1 : ( nIndex == 1 ) ? m_oCorner2 : m_oCorner3; }
            // The normals a smooth triangle blends between its corners.
        void                SetCornerNormals( const CVector &oNormal1, const CVector &oNormal2, const CVector &oNormal3 );
            // The normal shading uses at a point on the triangle (before a
            // double-sided triangle turns it toward the viewer).
        CVector             NormalAt( CVector &oPoint );

        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect );
        virtual BOOL        GetBounds( CVector &oMinimum, CVector &oMaximum );
        virtual BOOL        SurfaceNormal( CVector &oPoint, CVector &oNormal ) { oNormal = NormalAt( oPoint ); return TRUE; }
            // The corners' surface coordinates blended by barycentric
            // weights; FALSE if the triangle has none.
        virtual BOOL        SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV );
        void                SetCornerUVs( const FLT anUVs[ 6 ] );
        virtual CColor      Shade( CLight *pLight, CVector &oIntersect );
        virtual CColor      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );


#ifdef RAY_GUI
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
        virtual void    SetProperties( void );
#endif
};

#endif // __TRIANGLE_H__
