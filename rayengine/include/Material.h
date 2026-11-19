#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#ifdef _WINDOWS
    #include "SceneTreeView.h"
#endif

#include "Color.h"
#include "Rays.h"

class CScene;
class CLight;
class CObjects;

class CMaterial
{
   public:
        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CColor &oLight, CVector &oToLight, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect ) = 0;
        virtual CColor      GetColor( CScene &oScene, CRay &oViewIncident, CVector &oIntersect, CVector &oNormal, CObjects *pIntersect ) = 0;
        virtual LPCSTR      ClassType( void ) { return "CMaterial"; };
        virtual void        Out( CStream &oStream ) = 0;
        virtual void        SetColor( CColor &oColor ) = 0;
#ifdef _INC_WINDOWS
        virtual void        Properties( void ) = 0;
        virtual void        ToTree( CSceneTreeView *pTree, HTREEITEM hUnder ) = 0;
        virtual void        ToList( CListCtrl &oList, int nIndex, FLT nInfluence ) = 0;
#endif
};


#define DEFINE_MATERIAL(x)      static CMaterial *CFUNC CreateMe( CStream &oStream ) \
                                { \
                                    return new x( oStream ); \
                                } \
                                static CMaterial *CFUNC AddMe( void ) \
                                { \
                                    return new x; \
                                } \
                                x( CStream &oStream ) \
                                { \
                                    oStream >> (*this); \
                                } \
                                x( void ); \
                                virtual void Out( CStream &oStream ) \
                                { \
                                    oStream << (*this); \
                                } \
                                virtual LPCSTR ClassType( void ) { return #x; }

#define IMPLEMENT_MATERIAL(x,y) static CDynMaterial  oDynMaterial( x::CreateMe, x::AddMe, #x, #y );

typedef CMaterial *(CFUNC *LPADDFUNC)( void );

class CDynMaterial
{
    protected:
        static CDynMaterial    *m_pFirst;
        CString                 m_strObjectID;
        CMaterial      *(CFUNC *m_lpFunction)(CStream &oStream);
        CDynMaterial           *m_pNext;
        LPCSTR                  m_strAddName;
        LPADDFUNC               m_lpAddFunc;
    public:
                            CDynMaterial( CMaterial *(CFUNC *lpFunction)(CStream &oStream), LPADDFUNC lpAddFunc, LPCSTR nKey, LPCSTR strName );
        virtual            ~CDynMaterial( void );
        int                 Compare( CDynMaterial *oCompare );
        BOOL                Match( LPCSTR lpszCompare );
        CMaterial          *MakeMe( CStream &oStream );
        CDynMaterial       *GetFirst( void );
        CDynMaterial       *GetNext( void );
        LPCSTR              GetAddName( void ) { return m_strAddName; }
        LPADDFUNC           GetAddFunc( void ) { return m_lpAddFunc; }
};

class CDynMaterialList
{
    protected:
        CDynMaterial       *m_pFirst;
    public:
                            CDynMaterialList( void );
        virtual            ~CDynMaterialList( void );
        CMaterial          *Instantiate( LPCSTR lpszName, CStream &oStream );
#ifdef _INC_WINDOWS
        void                LoadAddList( CListCtrl &oList );
#endif
};

extern CDynMaterialList         *g_pMaterialList;   

#endif // __MATERIAL_H__
