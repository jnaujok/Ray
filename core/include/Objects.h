/*************************************************************************
*   CObjects header                                                      *
*                                                                        *
* This defines the CObject class, CDynObject class, and CDynList object  *
* for use as the basis for all 3-d objects. The CDynObject and CDynList  *
* and their associated macros make it simple and easy to add new objects *
* to the list of those available without lots of extra work.             *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __OBJECTS_H__
#define __OBJECTS_H__

#include "Globals.h"
#include "Vector.h"
#include "Color.h"
#include "Rays.h"
#include "Globals.h"
#include "MaterialList.h"

#include <memory>

#ifdef RAY_GUI
    #include "SceneTreeView.h"
#endif

class CBumpMap;
class CLight;
class CEnvironment;
class CScene;

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

struct STexturePoint;

    // An affine map, a 3x3 matrix and an offset (row by row, each row's
    // offset last), from scene points to the space an object's textures are
    // looked up in: a mesh loaded from a file maps them back into the file's
    // own coordinates, so its textures move, turn and grow with it.
struct STextureFrame
{
    FLT                 anMatrix[ 12 ];

    CVector             Apply( const CVector &oPoint ) const
    {
        CVector oIn( oPoint );

        return CVector( ( anMatrix[ 0 ] * oIn.GetX() ) + ( anMatrix[ 1 ] * oIn.GetY() ) + ( anMatrix[ 2 ] * oIn.GetZ() ) + anMatrix[ 3 ],
                        ( anMatrix[ 4 ] * oIn.GetX() ) + ( anMatrix[ 5 ] * oIn.GetY() ) + ( anMatrix[ 6 ] * oIn.GetZ() ) + anMatrix[ 7 ],
                        ( anMatrix[ 8 ] * oIn.GetX() ) + ( anMatrix[ 9 ] * oIn.GetY() ) + ( anMatrix[ 10 ] * oIn.GetZ() ) + anMatrix[ 11 ] );
    }
};

class CObjects
{
    protected:
        CMaterialList       m_oMaterials;
        BOOL                m_bOwnsMaterials;   // FALSE for a mesh's faces, which share the mesh's
        std::shared_ptr<const CBumpMap> m_pBump;    // tilts the shading normal, if set (a mesh's faces share the mesh's)
            // The materials BaseShade uses instead of the object's own, when
            // set: a csg shading a surface with its part's materials.
        CMaterialList      *m_pShadeMaterials;
            // Where textures are looked up, if not the scene's own space (a
            // mesh from a file, and its faces).
        std::shared_ptr<const STextureFrame> m_pTextureFrame;
#ifdef RAY_GUI
        HTREEITEM           m_hObjectItem;
        HTREEITEM           m_hMaterialTree;
        CSceneTreeView     *m_poTree;
#endif

    public:
                            CObjects( void );
        virtual            ~CObjects( void );
      
            // The object as JSON for a scene file: its fields go into oNode,
            // which already holds its "type". FromJson reads them back and
            // returns FALSE, with RayJson::Fail, if one is missing or bad.
        virtual void        ToJson( IASLib::CObjectNode &oNode ) = 0;
        virtual BOOL        FromJson( const IASLib::CJsonNode &oNode ) = 0;
        virtual LPCSTR      ClassType( void ) { return "CObjects"; }
        virtual BOOL        IsALight( void ) { return FALSE; }
        virtual BOOL        IsCamera( void ) { return FALSE; }
        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect ) {return FALSE;}
            // The surface coordinates (u, v) of a point on the object, for
            // textures. FALSE if the object has none.
        virtual BOOL        SurfaceCoords( CVector &oPoint, FLT &nU, FLT &nV ) { (void)oPoint; nU = nV = 0.0; return FALSE; }
            // The color the object's own data gives the surface at a point (a
            // face's color from a colored model file), for the faceColors
            // texture. FALSE if it gives none.
        virtual BOOL        SurfaceColor( CVector &oPoint, CColor &oColor ) { (void)oPoint; (void)oColor; return FALSE; }
            // A scene point in the space the object's textures are looked up
            // in: the point itself, unless the object has a texture frame.
        virtual CVector     TextureSpace( const CVector &oPoint ) const
                            { return m_pTextureFrame ? m_pTextureFrame->Apply( oPoint ) : oPoint; }
        virtual void        SetTextureFrame( const std::shared_ptr<const STextureFrame> &pFrame ) { m_pTextureFrame = pFrame; }
            // Everything a texture looks up at a point on the object: the
            // point in texture space, its surface coordinates and its color.
        void                TextureAt( CVector &oPoint, STexturePoint &oAt );
            // Intersect, unless this is pIgnore: the object a shadow or
            // reflected ray leaves from. An object made of parts (a mesh)
            // skips only the part pIgnore names, so it can shadow itself.
        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect, CObjects *pIgnore )
                            { return ( pIgnore == this ) ? FALSE : Intersect( oRay, oIntersect ); }
            // Whether the ray hits the object at all before its t (for
            // shadows), ignoring pIgnore as Intersect does. Any hit will do,
            // so an object made of parts can stop at the first.
        virtual BOOL        Occludes( CRay &oRay, CObjects *pIgnore )
                            { CVector oIntersect; return Intersect( oRay, oIntersect, pIgnore ); }
            // The object's bounding box, for the scene's tree. FALSE if it
            // has none (an infinite plane).
        virtual BOOL        GetBounds( CVector &oMinimum, CVector &oMaximum ) { (void)oMinimum; (void)oMaximum; return FALSE; }
            // The outward normal of the surface at a point on it, as Shade
            // shades it before any bump map. FALSE if the object can't say.
        virtual BOOL        SurfaceNormal( CVector &oPoint, CVector &oNormal ) { (void)oPoint; (void)oNormal; return FALSE; }
            // The part of the object its last Intersect hit (a mesh's face,
            // a csg's shape), and whether that part's normal is turned
            // inside out there (a csg's subtracted shape). Itself for a
            // simple object.
        virtual CObjects   *HitPart( BOOL &bFlip ) { bFlip = FALSE; return this; }
            // Whether the object encloses a volume, every ray crossing in and
            // out of it, so a csg can use it.
        virtual BOOL        IsClosed( void ) { return FALSE; }
            // Which face of the mesh pMesh this object stands for (a mesh
            // shades a face through a stand-in made for it), or -1.
        virtual int         MeshFaceOf( const CObjects *pMesh ) const { (void)pMesh; return -1; }
            // Whether this and pOther are the same surface: the same object,
            // or stand-ins for the same face of a mesh.
        virtual BOOL        SameSurface( const CObjects *pOther ) const { return ( pOther == this ) ? TRUE : FALSE; }
        CMaterialList      &GetMaterials( void ) { return m_oMaterials; }
        virtual CColor      Shade( CLight *pLight, CVector &oIntersect ) { CColor oTemp( 0, 0, 0 ); return oTemp; }
        virtual CColor      Shade( CScene &oScene, CRay &oRay, CVector &oIntersect ) { CColor oTemp( 0, 0, 0 ); return oTemp; }
#ifdef RAY_GUI
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder ) = 0;
        virtual void        SetProperties( void ) { return; }
        virtual void        SetMaterials( void );

        BOOL                m_bSelected;
        BOOL                m_bExpanded;
    protected:
        void                MatToTree( CSceneTreeView *pTree, HTREEITEM hUnder );

    public:
#ifdef _DEBUG
        virtual void        Dump( CDumpContext& dc ) const;
#endif
#endif
            // The object's bump or normal map, if it has one ("bump" in
            // JSON, written and read by each object class through these).
        const std::shared_ptr<const CBumpMap> &GetBump( void ) const { return m_pBump; }
        virtual void        SetBump( const std::shared_ptr<const CBumpMap> &pBump ) { m_pBump = pBump; }
        void                BumpToJson( IASLib::CObjectNode &oNode ) const;
        BOOL                BumpFromJson( const IASLib::CJsonNode &oNode );
    protected:
        CColor              BaseShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal );
            // Adds what an area light (nLight in the scene) gives the point.
        void                AreaLightShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal, CLight *pLight,
                                            int nLight, CColor &oReturn );
            // Adds the light the environment gives the point (when it has
            // samples), from directions over the half of the sky above it.
        void                EnvironmentShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal,
                                              const CEnvironment &oEnvironment, CColor &oReturn );
            // Adds the light bounced off other surfaces nearby (the scene's
            // "indirect" settings) to the point's diffuse materials.
        void                IndirectShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal, CColor &oReturn );
};


#ifndef DYNAMIC_OBJECTS
class CDynObject
{
    protected:
        static CDynObject  *m_pFirst;
        IASLib::CString             m_strObjectID;
        CObjects   *(CFUNC *m_lpCreateEmpty)(void);
        LPCSTR              m_strJsonType;      // Its "type" in JSON scene files.
        CDynObject         *m_pNext;
    public:
                            CDynObject( CObjects *(CFUNC *lpCreateEmpty)(void), LPCSTR nKey, LPCSTR strJsonType );
        virtual            ~CDynObject( void );
        int                 Compare( CDynObject *oCompare );
        BOOL                Match( LPCSTR lpszCompare );
        CObjects           *MakeEmpty( void );
        LPCSTR              GetJsonType( void ) { return m_strJsonType; }
        CDynObject         *GetFirst( void );
        CDynObject         *GetNext( void );
};

class CDynList
{
    protected:
        CDynObject         *m_pFirst;
    public:
                            CDynList( void );
        virtual            ~CDynList( void );
            // A new, default object of a JSON scene file's "type", or NULL.
        CObjects           *Create( LPCSTR strJsonType );
            // The JSON "type" of an object class (its ClassType()), or NULL.
        LPCSTR              JsonTypeOf( LPCSTR strClassType );
};

extern CDynList    *g_pDynamicList;   

#define DYNAMIC_OBJECTS( x )    static CObjects* CFUNC CreateEmpty( void ) \
                                {\
                                    return new x;\
                                } \
                                virtual void ToJson( IASLib::CObjectNode &oNode ); \
                                virtual BOOL FromJson( const IASLib::CJsonNode &oNode ); \
                                virtual LPCSTR ClassType( void ) { return #x; };
                            
#define DYNAMIC_IMPLEMENTS( x, json )   static CDynObject  oDynObject( x::CreateEmpty, #x, json );
#endif

#endif      // __OBJECTS_H__


