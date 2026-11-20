/*************************************************************************
*   CRay                                                                 *
*                                                                        *
*       This class defines a single ray to be cast into the scene. A ray *
* consists of a direction (the i, j, k values) and a starting point (the *
* x, y, z values). It also has the time of closest intersection stored   *
* in it to make the depth queueing of rays much simpler since we are     *
* only ever worried about the closest intersection with the ray.         *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __RAYS_H__
#define __RAYS_H__

class CScene;
class CVector;

class CRay
{
    protected:
        FLT             m_nI;
        FLT             m_nJ;
        FLT             m_nK;
        FLT             m_nX;
        FLT             m_nY;
        FLT             m_nZ;
        FLT             m_nT;
        FLT             m_nInfluence;
        long            m_nPixelX;
        long            m_nPixelY;
        long            m_nDepth;
            // Where on the image a primary ray was made for (not sent to
            // engines; the master uses it to remake the ray per channel).
        FLT             m_nImageX;
        FLT             m_nImageY;
    public:
                        CRay( void );
                        CRay( const CRay &oSource );
        virtual        ~CRay( void );
        virtual void    CreateRay( CScene &oScene, int nX, int nY, FLT nInfluence = 1.0 );
        virtual void    CreateRayAt( CScene &oScene, FLT nImageX, FLT nImageY, int nTagX, int nTagY, FLT nInfluence = 1.0,
                                     int nChannel = 0 );
        FLT             GetImageX( void ) const { return m_nImageX; }
        FLT             GetImageY( void ) const { return m_nImageY; }
        virtual void    CreateRay( CVector &oVector, CVector &oOrigin, FLT nInfluence, int nX, int nY );


        CRay           &operator =( const CRay &oSource );
        virtual FLT     GetI(void) {return m_nI;}
        virtual FLT     GetJ(void) {return m_nJ;}
        virtual FLT     GetK(void) {return m_nK;}
        virtual FLT     GetX(void) {return m_nX;}
        virtual FLT     GetY(void) {return m_nY;}
        virtual FLT     GetZ(void) {return m_nZ;}
        virtual FLT     GetT(void) {return m_nT;}
        virtual FLT     GetInfluence( void ) { return m_nInfluence; }
        virtual void    SetT( FLT nNewT );
            // Sets every field (as the wire protocol carries them), with a
            // depth of 0.
        void            Set( FLT nX, FLT nY, FLT nZ, FLT nI, FLT nJ, FLT nK, FLT nT, FLT nInfluence, int nPixelX, int nPixelY );
        virtual int     GetPixelX( void ) { return m_nPixelX; }
        virtual int     GetPixelY( void ) { return m_nPixelY; }
        void            Increment( void ) { m_nDepth++; }
        virtual int     GetDepth( void ) { return m_nDepth; }
        virtual void    SetDepth( int nDepth ) { m_nDepth = nDepth; }

};

#endif // __RAYS_H__


