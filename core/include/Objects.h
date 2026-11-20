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

class CObjects
{
    protected:
        CMaterialList       m_oMaterials;
        BOOL                m_bOwnsMaterials;   // FALSE for a mesh's faces, which share the mesh's
        std::shared_ptr<const CBumpMap> m_pBump;    // tilts the shading normal, if set (a mesh's faces share the mesh's)
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


