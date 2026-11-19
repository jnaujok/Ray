/*************************************************************************
*   CMesh                                                                *
*                                                                        *
*   A surface made of triangles that share their vertices: a list of     *
*   vertices, and faces given as three vertex indices each. The faces    *
*   are ordinary CTriangles, owned by the mesh and sharing its           *
*   materials. Each vertex gets a normal, the sum of the normals of      *
*   every face it belongs to, so a smooth mesh shades as one curved      *
*   surface. The corner order of each face decides its front, as for     *
*   CTriangle.                                                           *
*                                                                        *
*   A ray that misses the mesh's bounding box skips all its faces. Rays  *
*   leaving one face (shadows, reflections) skip only that face, so a    *
*   mesh shadows and reflects itself.                                    *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __MESH_H__
#define __MESH_H__

#include "Objects.h"
#include "Triangle.h"

#include <vector>

class CMesh : public CObjects
{
    protected:
        std::vector<CVector>    m_aoVertices;
        std::vector<int>        m_anFaces;          // three vertex indices a face
        BOOL                    m_bDoubleSided;
        BOOL                    m_bSmooth;
        CColor                  m_oColor;

        std::vector<CTriangle *> m_apFaces;         // built from the above
        CVector                 m_oMinimum;         // the bounding box
        CVector                 m_oMaximum;
        CTriangle              *m_pHitFace;         // the face Intersect hit last

        void                    DeleteFaces( void );
        BOOL                    HitsBox( CRay &oRay );

    public:
                                DYNAMIC_OBJECTS( CMesh );

                                CMesh( void );
        virtual                ~CMesh( void );

            /// Replaces the geometry and rebuilds the faces and normals.
            /// Faces whose corners are in a line are left out. Returns FALSE
            /// if an index is out of range.
        BOOL                    SetGeometry( const std::vector<CVector> &aoVertices, const std::vector<int> &anFaces,
                                             BOOL bSmooth, BOOL bDoubleSided );
        int                     GetFaceCount( void ) const { return (int)m_apFaces.size(); }
            /// The normal face nFace shades with at a point on it.
        CVector                 FaceNormalAt( int nFace, CVector oPoint ) { return m_apFaces[ (size_t)nFace ]->NormalAt( oPoint ); }

        virtual BOOL            Intersect( CRay &oRay, CVector &oIntersect );
        virtual BOOL            Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore );
        virtual CColor          Shade( CScene &oScene, CRay &oRay, CVector &oIntersect );

#ifdef RAY_GUI
        virtual void            ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif

    private:
                                CMesh( const CMesh & );
        CMesh                  &operator =( const CMesh & );
};

#endif // __MESH_H__
