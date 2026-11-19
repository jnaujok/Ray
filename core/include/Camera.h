/*************************************************************************
*   CCamera                                                              *
*                                                                        *
*       This is the base class for all cameras. This basic camera simply *
* defines a standard 4/3 ratio NTSC type screen.                         *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "Objects.h"
#include "Vector.h"
#include "Matrix.h"

class CCamera : public CObjects
{
    protected:
        CVector     m_oPosition;
        CVector     m_oPointAt;
        CVector     m_oUp;
        FLT         m_nVerticalAngle;
        FLT         m_nHorizontalAngle;
        FLT         m_nAspectRatio;
        FLT         m_nXRotate;
        FLT         m_nYRotate;
    public:
                    CCamera( void );
                    CCamera( CVector oPosition, CVector oPointAt, FLT nVerticalAngle, FLT nHorizontalAngle, FLT nAspectRatio = 0.0, CVector *poUp = NULL );
                    CCamera( const CCamera &oSource );
        virtual    ~CCamera( void );

        virtual BOOL    IsCamera( void ) { return TRUE; }

        CCamera    &operator =( const CCamera &oSource );

        FLT         GetHorizontalAngle( void ) { return m_nHorizontalAngle; }
        FLT         GetVerticalAngle( void ) { return m_nVerticalAngle; }
        CVector     GetPosition( void ) { return m_oPosition; }
        CVector     PointAt( void ) { return m_oPointAt; }

        DYNAMIC_OBJECTS( CCamera );

        friend CRayStream  &operator >>( CRayStream &oStream, CCamera &oCamera );
        friend CRayStream  &operator <<( CRayStream &oStream, CCamera &oCamera );

        void        CalcRotate( void );
        FLT         GetXRotate( void ) { return m_nXRotate; }
        FLT         GetYRotate( void ) { return m_nYRotate; }

#ifdef RAY_GUI
        virtual void    SetProperties( void );
        virtual void    ToTree( CSceneTreeView *pTree, HTREEITEM hUnder );
#endif
};

#endif
