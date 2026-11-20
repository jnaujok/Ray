/*************************************************************************
*   Editor math                                                          *
*                                                                        *
*   Small vector and matrix helpers for the scene editor's view: plain   *
*   doubles, so the view code needs none of the tracer's classes. The    *
*   matrices are OpenGL's: 4x4, column by column.                        *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __EDITORMATH_H__
#define __EDITORMATH_H__

#include <cmath>

namespace RayEditor
{
    const double EDITOR_PI = 3.14159265358979323846;

    struct SVec3
    {
        double  x = 0.0, y = 0.0, z = 0.0;

                SVec3( void ) {}
                SVec3( double nX, double nY, double nZ ) : x( nX ), y( nY ), z( nZ ) {}

        SVec3   operator +( const SVec3 &o ) const { return SVec3( x + o.x, y + o.y, z + o.z ); }
        SVec3   operator -( const SVec3 &o ) const { return SVec3( x - o.x, y - o.y, z - o.z ); }
        SVec3   operator *( double n ) const { return SVec3( x * n, y * n, z * n ); }
        SVec3   operator -( void ) const { return SVec3( -x, -y, -z ); }
        double  Dot( const SVec3 &o ) const { return ( x * o.x ) + ( y * o.y ) + ( z * o.z ); }
        SVec3   Cross( const SVec3 &o ) const { return SVec3( ( y * o.z ) - ( z * o.y ), ( z * o.x ) - ( x * o.z ), ( x * o.y ) - ( y * o.x ) ); }
        double  Length( void ) const { return std::sqrt( Dot( *this ) ); }
        SVec3   Normalized( void ) const { double n = Length(); return ( n > 0.0 ) ? ( *this * ( 1.0 / n ) ) : *this; }
    };

        // A vector that is neither zero nor along oAxis, at right angles to it.
    inline SVec3 Perpendicular( const SVec3 &oAxis )
    {
        SVec3 oOther = ( std::fabs( oAxis.x ) < 0.9 ) ? SVec3( 1, 0, 0 ) : SVec3( 0, 1, 0 );

        return oAxis.Cross( oOther ).Normalized();
    }

        // Turns a vector about x, then y, then z, by angles in degrees, as a
        // box's and a model's "rotation" do in the tracer.
    inline SVec3 RotateXYZ( const SVec3 &oIn, double nXDeg, double nYDeg, double nZDeg )
    {
        double nX = nXDeg * EDITOR_PI / 180.0, nY = nYDeg * EDITOR_PI / 180.0, nZ = nZDeg * EDITOR_PI / 180.0;
        SVec3  v = oIn;
        double nA, nB;

        nA = ( v.y * std::cos( nX ) ) - ( v.z * std::sin( nX ) );
        nB = ( v.y * std::sin( nX ) ) + ( v.z * std::cos( nX ) );
        v.y = nA; v.z = nB;
        nA = ( v.x * std::cos( nY ) ) + ( v.z * std::sin( nY ) );
        nB = ( v.z * std::cos( nY ) ) - ( v.x * std::sin( nY ) );
        v.x = nA; v.z = nB;
        nA = ( v.x * std::cos( nZ ) ) - ( v.y * std::sin( nZ ) );
        nB = ( v.x * std::sin( nZ ) ) + ( v.y * std::cos( nZ ) );
        v.x = nA; v.y = nB;
        return v;
    }

        // OpenGL's perspective matrix (as gluPerspective makes it).
    inline void Perspective( double nFovYDeg, double nAspect, double nNear, double nFar, double anOut[ 16 ] )
    {
        double f = 1.0 / std::tan( nFovYDeg * EDITOR_PI / 360.0 );

        for ( int i = 0; i < 16; i++ )
            anOut[ i ] = 0.0;
        anOut[ 0 ] = f / nAspect;
        anOut[ 5 ] = f;
        anOut[ 10 ] = ( nFar + nNear ) / ( nNear - nFar );
        anOut[ 11 ] = -1.0;
        anOut[ 14 ] = ( 2.0 * nFar * nNear ) / ( nNear - nFar );
    }

        // OpenGL's view matrix for an eye with the given unit vectors: the
        // image's right and top edges, and where it looks. Any handedness
        // works: the scene's left-handed coordinates come out with right to
        // the right and top at the top, as the tracer draws them.
    inline void ViewFromBasis( const SVec3 &oEye, const SVec3 &s, const SVec3 &u, const SVec3 &f, double anOut[ 16 ] )
    {
        anOut[ 0 ] = s.x;  anOut[ 4 ] = s.y;  anOut[ 8 ] = s.z;   anOut[ 12 ] = -s.Dot( oEye );
        anOut[ 1 ] = u.x;  anOut[ 5 ] = u.y;  anOut[ 9 ] = u.z;   anOut[ 13 ] = -u.Dot( oEye );
        anOut[ 2 ] = -f.x; anOut[ 6 ] = -f.y; anOut[ 10 ] = -f.z; anOut[ 14 ] = f.Dot( oEye );
        anOut[ 3 ] = 0.0;  anOut[ 7 ] = 0.0;  anOut[ 11 ] = 0.0;  anOut[ 15 ] = 1.0;
    }
}

#endif // __EDITORMATH_H__
