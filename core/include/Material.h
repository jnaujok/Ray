#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#ifdef RAY_GUI
    #include "SceneTreeView.h"
#endif

#include "Color.h"
#include "Rays.h"
#include "Texture.h"

#include <memory>

class CScene;
class CLight;
class CObjects;

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

class CMaterial
{
    protected:
            // A texture to use in place of the material's own color, or NULL.
        std::unique_ptr<CTexture>   m_pTexture;

            // The color at a point: the texture's there, if there is one,
            // else oColor. pObject gives the point's surface coordinates.
        CColor              ColorAt( const CColor &oColor, CVector &oPoint, CObjects *pObject );

   public:
        virtual            ~CMaterial( void ) {}

            // The texture, as JSON: TextureToJson adds a "texture" field to
            // oNode if there is a texture; TextureFromJson reads oNode's
            // "texture" field, if it has one.
        void                TextureToJson( IASLib::CObjectNode &oNode ) const;
        BOOL                TextureFromJson( const IASLib::CJsonNode &oNode );

        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect ) = 0;
        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect ) = 0;
        virtual LPCSTR      ClassType( void ) { return "CMaterial"; };
            // Whether the environment's light (CEnvironment, with samples)
            // shades it: diffuse materials. Light from the sky's other
            // directions reaches shiny ones as their reflections.
        virtual BOOL        LitBySky( void ) { return FALSE; }
            // The material as JSON for a scene file: its fields go into oNode,
            // which already holds its "type". FromJson reads them back and
            // returns FALSE, with RayJson::Fail, if one is missing or bad.
        virtual void        ToJson( IASLib::CObjectNode &oNode ) = 0;
        virtual BOOL        FromJson( const IASLib::CJsonNode &oNode ) = 0;
        virtual void        SetColor( CColor &oColor ) = 0;
#ifdef RAY_GUI
        virtual void        Properties( void ) = 0;
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder ) = 0;
        virtual void        ToList( CListCtrl &oList, int nIndex, FLT nInfluence ) = 0;
#endif
};


#define DEFINE_MATERIAL(x)      static CMaterial *CFUNC AddMe( void ) \
                                { \
                                    return new x; \
                                } \
                                x( void ); \
                                virtual void ToJson( IASLib::CObjectNode &oNode ); \
                                virtual BOOL FromJson( const IASLib::CJsonNode &oNode ); \
                                virtual LPCSTR ClassType( void ) { return #x; }

#define IMPLEMENT_MATERIAL(x,y,json) static CDynMaterial  oDynMaterial( x::AddMe, #x, #y, json );

typedef CMaterial *(CFUNC *LPADDFUNC)( void );

class CDynMaterial
{
    protected:
        static CDynMaterial    *m_pFirst;
        IASLib::CString                 m_strObjectID;
        CDynMaterial           *m_pNext;
        LPCSTR                  m_strAddName;
        LPADDFUNC               m_lpAddFunc;
        LPCSTR                  m_strJsonType;      // Its "type" in JSON scene files.
    public:
                            CDynMaterial( LPADDFUNC lpAddFunc, LPCSTR nKey, LPCSTR strName, LPCSTR strJsonType );
        virtual            ~CDynMaterial( void );
        int                 Compare( CDynMaterial *oCompare );
        BOOL                Match( LPCSTR lpszCompare );
        CDynMaterial       *GetFirst( void );
        CDynMaterial       *GetNext( void );
        LPCSTR              GetAddName( void ) { return m_strAddName; }
        LPADDFUNC           GetAddFunc( void ) { return m_lpAddFunc; }
        LPCSTR              GetJsonType( void ) { return m_strJsonType; }
};

class CDynMaterialList
{
    protected:
        CDynMaterial       *m_pFirst;
    public:
                            CDynMaterialList( void );
        virtual            ~CDynMaterialList( void );
            // A new, default material of a JSON scene file's "type", or NULL.
        CMaterial          *Create( LPCSTR strJsonType );
            // The JSON "type" of a material class (its ClassType()), or NULL.
        LPCSTR              JsonTypeOf( LPCSTR strClassType );
#ifdef RAY_GUI
        void                LoadAddList( CListCtrl &oList );
#endif
};

extern CDynMaterialList         *g_pMaterialList;   

#endif // __MATERIAL_H__
