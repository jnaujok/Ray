#include "ViewCamera.h"

#include <algorithm>

namespace RayEditor
{
    CViewCamera::CViewCamera( void )
        : m_oEye( 0.0, 5.0, -15.0 ), m_oTarget( 0.0, 0.0, 0.0 ), m_oUp( 0.0, 1.0, 0.0 ), m_nFovY( 40.0 )
    {
    }

    void CViewCamera::Set( const SVec3 &oEye, const SVec3 &oTarget, const SVec3 &oUp, double nFovY )
    {
        m_oEye = oEye;
        m_oTarget = oTarget;
        m_oUp = ( oUp.Length() > 0.0 ) ? oUp.Normalized() : SVec3( 0.0, 1.0, 0.0 );
        m_nFovY = (std::min)( 170.0, (std::max)( 1.0, nFovY ) );
        if ( ( m_oTarget - m_oEye ).Length() <= 0.0 )
            m_oTarget = m_oEye + SVec3( 0.0, 0.0, 1.0 );
    }

    void CViewCamera::Basis( SVec3 &oForward, SVec3 &oRight, SVec3 &oTop ) const
    {
        oForward = ( m_oTarget - m_oEye ).Normalized();

        SVec3 oUp = m_oUp;

        if ( std::fabs( oForward.Dot( oUp ) ) > 0.9999 )
            oUp = Perpendicular( oForward );
            // The scene's coordinates are left-handed, as the tracer's camera
            // takes them (CCamera::CalcRotate): right is up x forward.
        oRight = oUp.Cross( oForward ).Normalized();
        oTop = oForward.Cross( oRight ).Normalized();
    }

    void CViewCamera::Orbit( double nYaw, double nPitch )
    {
        SVec3   oUp = m_oUp.Normalized();
        SVec3   oOffset = m_oEye - m_oTarget;
        double  nDistance = oOffset.Length();

        if ( nDistance <= 0.0 )
            return;

            // Yaw: about the up direction.
        double nYawRad = nYaw * EDITOR_PI / 180.0;
        SVec3  oAlong = oUp * oOffset.Dot( oUp );
        SVec3  oAcross = oOffset - oAlong;
        SVec3  oSide = oUp.Cross( oAcross );

        oOffset = oAlong + ( oAcross * std::cos( nYawRad ) ) + ( oSide * std::sin( nYawRad ) );

            // Pitch: toward or away from the up direction, short of it.
        double nElevation = std::asin( (std::max)( -1.0, (std::min)( 1.0, oOffset.Normalized().Dot( oUp ) ) ) ) * 180.0 / EDITOR_PI;
        double nNew = (std::max)( -89.0, (std::min)( 89.0, nElevation + nPitch ) );
        SVec3  oFlat = ( oOffset - ( oUp * oOffset.Dot( oUp ) ) ).Normalized();

        if ( oFlat.Length() <= 0.0 )
            oFlat = Perpendicular( oUp );
        oOffset = ( ( oFlat * std::cos( nNew * EDITOR_PI / 180.0 ) ) + ( oUp * std::sin( nNew * EDITOR_PI / 180.0 ) ) ) * nDistance;
        m_oEye = m_oTarget + oOffset;
    }

    void CViewCamera::Pan( double nX, double nY )
    {
        SVec3   oForward, oRight, oTop;
        double  nDistance = ( m_oTarget - m_oEye ).Length();
        double  nHeight = 2.0 * nDistance * std::tan( m_nFovY * EDITOR_PI / 360.0 );
        SVec3   oMove = ( oRight * 0.0 );

        Basis( oForward, oRight, oTop );
        oMove = ( oRight * ( -nX * nHeight ) ) + ( oTop * ( nY * nHeight ) );
        m_oEye = m_oEye + oMove;
        m_oTarget = m_oTarget + oMove;
    }

    void CViewCamera::Zoom( double nFactor )
    {
        SVec3   oOffset = m_oEye - m_oTarget;
        double  nDistance = oOffset.Length() * nFactor;

        if ( nDistance < 1e-4 )
            nDistance = 1e-4;
        m_oEye = m_oTarget + ( oOffset.Normalized() * nDistance );
    }

    void CViewCamera::Frame( const SVec3 &oMinimum, const SVec3 &oMaximum )
    {
        SVec3   oCenter = ( oMinimum + oMaximum ) * 0.5;
        double  nRadius = (std::max)( 1e-3, ( oMaximum - oMinimum ).Length() * 0.5 );
        double  nDistance = nRadius / std::sin( (std::min)( 80.0, m_nFovY * 0.5 ) * EDITOR_PI / 180.0 );
        SVec3   oBack = ( m_oEye - m_oTarget ).Normalized();

        if ( oBack.Length() <= 0.0 )
            oBack = SVec3( 0.0, 0.3, -1.0 ).Normalized();
        m_oTarget = oCenter;
        m_oEye = oCenter + ( oBack * nDistance );
    }

    SVec3 CViewCamera::RayDirection( double nScreenX, double nScreenY, double nAspect ) const
    {
        SVec3   oForward, oRight, oTop;
        double  nHalfHeight = std::tan( m_nFovY * EDITOR_PI / 360.0 );
        double  nHalfWidth = nHalfHeight * nAspect;

        Basis( oForward, oRight, oTop );
        return ( oForward + ( oRight * ( nScreenX * nHalfWidth ) ) + ( oTop * ( nScreenY * nHalfHeight ) ) ).Normalized();
    }

    void CViewCamera::PixelRay( double nX, double nY, int nWidth, int nHeight, SVec3 &oOrigin, SVec3 &oDirection ) const
    {
        double nW = ( nWidth > 0 ) ? nWidth : 1;
        double nH = ( nHeight > 0 ) ? nHeight : 1;
        double nScreenX = ( ( ( nX + 0.5 ) / nW ) * 2.0 ) - 1.0;
        double nScreenY = 1.0 - ( ( ( nY + 0.5 ) / nH ) * 2.0 );

        oOrigin = m_oEye;
        oDirection = RayDirection( nScreenX, nScreenY, nW / nH );
    }

    void CViewCamera::Projection( double nAspect, double nNear, double nFar, double anOut[ 16 ] ) const
    {
        Perspective( m_nFovY, ( nAspect > 0.0 ) ? nAspect : 1.0, nNear, nFar, anOut );
    }

    void CViewCamera::View( double anOut[ 16 ] ) const
    {
        SVec3 oForward, oRight, oTop;

        Basis( oForward, oRight, oTop );
        ViewFromBasis( m_oEye, oRight, oTop, oForward, anOut );
    }
}
