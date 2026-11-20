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

#include <stdint.h>

class CScene;
class CVector;
class CObjects;

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
            // What a primary ray's color counts for: a lens sampling only
            // part of its rear element scales by the share it samples.
        FLT             m_nWeight;
        long            m_nPixelX;
        long            m_nPixelY;
        long            m_nDepth;
            // Where on the image a primary ray was made for (not sent to
            // engines; the master uses it to remake the ray per channel).
        FLT             m_nImageX;
        FLT             m_nImageY;
            // The transparent object the ray is travelling through, or NULL
            // (outside everything). Set where a ray goes into or out of
            // glass, so the far side knows how far it went inside.
        const CObjects *m_pInside;
            // Where the primary ray fell in its pixel (each 0 up to 1), and a
            // key made from the pixel and the bounces since: rough surfaces
            // and area lights pick their directions and points from these
            // (SamplePoint), so rays spread evenly over a pixel spread evenly
            // over them too.
        FLT             m_nSampleU;
        FLT             m_nSampleV;
        uint64_t        m_nSampleKey;
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
        void            SetInfluence( FLT nInfluence ) { m_nInfluence = nInfluence; }
            /// The factor a primary ray's traced color is multiplied by (1
            /// unless a lens sampled only part of its rear element).
        FLT             GetWeight( void ) const { return m_nWeight; }
        virtual void    SetT( FLT nNewT );
            // Sets every field (as the wire protocol carries them), with a
            // depth of 0.
        void            Set( FLT nX, FLT nY, FLT nZ, FLT nI, FLT nJ, FLT nK, FLT nT, FLT nInfluence, int nPixelX, int nPixelY );
        virtual int     GetPixelX( void ) { return m_nPixelX; }
        virtual int     GetPixelY( void ) { return m_nPixelY; }
        void            Increment( void ) { m_nDepth++; }
        virtual int     GetDepth( void ) { return m_nDepth; }
        virtual void    SetDepth( int nDepth ) { m_nDepth = nDepth; }
        const CObjects *GetInside( void ) const { return m_pInside; }
        void            SetInside( const CObjects *pInside ) { m_pInside = pInside; }
            /// A ray made from another at a surface (reflected or refracted)
            /// takes a sample point hashed from its parent's (independent of
            /// it) and a key for the branch.
        void            InheritSample( const CRay &oParent, uint64_t nBranch );
            /// The point of the unit square this ray uses for nPurpose
            /// (RaySampling::EPurpose): its sample point, shifted (wrapping
            /// round) and turned by a hash of its key, its depth and the
            /// purpose. Over a pixel's rays the points stay as evenly spread
            /// as the sampler made them.
        void            SamplePoint( uint64_t nPurpose, FLT &nU, FLT &nV ) const;

};

#endif // __RAYS_H__


