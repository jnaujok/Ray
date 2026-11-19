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
#include "RayStream.h"
#include "MaterialList.h"

#ifdef RAY_GUI
    #include "SceneTreeView.h"
#endif

class CLight;
class CScene;

class CObjects
{
    protected:
        CMaterialList       m_oMaterials;
#ifdef RAY_GUI
        HTREEITEM           m_hObjectItem;
        HTREEITEM           m_hMaterialTree;
        CSceneTreeView     *m_poTree;
#endif

    public:
                            CObjects( void );
        virtual            ~CObjects( void );
      
        virtual void        Serialize( CRayStream &oStream, BOOL ) = 0;
        virtual LPCSTR      ClassType( void ) { return "CObjects"; }
        virtual BOOL        IsALight( void ) { return FALSE; }
        virtual BOOL        IsCamera( void ) { return FALSE; }
        virtual BOOL        Intersect( CRay &oRay, CVector &oIntersect ) {return FALSE;}
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
    protected:
        CColor              BaseShade( CScene &oScene, CRay &oRay, CVector &oIntersect, CVector &oNormal );
};      


#ifndef DYNAMIC_OBJECTS
class CDynObject
{
    protected:
        static CDynObject  *m_pFirst;
        IASLib::CString             m_strObjectID;
        CObjects   *(CFUNC *m_lpFunction)(CRayStream &oStream);
        CDynObject         *m_pNext;
    public:
                            CDynObject( CObjects *(CFUNC *lpFunction)(CRayStream &oStream), LPCSTR nKey );
        virtual            ~CDynObject( void );
        int                 Compare( CDynObject *oCompare );
        BOOL                Match( LPCSTR lpszCompare );
        CObjects           *MakeMe( CRayStream &oStream );
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
        CObjects           *Instantiate( LPCSTR lpszName, CRayStream &oStream );
};

extern CDynList    *g_pDynamicList;   

#define DYNAMIC_OBJECTS( x )    static CObjects* CFUNC CreateMe( CRayStream &oStream ) \
                                {\
                                    return new x( oStream );\
                                } \
                                x( CRayStream &oStream );\
                                virtual void Serialize( CRayStream &oStream, BOOL bLoad ) \
                                { \
                                    if (bLoad) \
                                        oStream >> *this; \
                                    else \
                                        oStream << *this; \
                                } \
                                virtual LPCSTR ClassType( void ) { return #x; };
                            
#define DYNAMIC_IMPLEMENTS( x )   static CDynObject  oDynObject( x::CreateMe, #x );\
                                x::x( CRayStream &oStream )\
                                {\
                                    oStream >> *this; \
                                }
#endif

#endif      // __OBJECTS_H__


