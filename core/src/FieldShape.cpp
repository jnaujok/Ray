/*************************************************************************
*   CFieldShape                                                          *
*                                                                        *
*   The base of the shapes defined by a number at every point. See       *
*   FieldShape.h.                                                        *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "FieldShape.h"
#include "SceneJson.h"

#include <math.h>


CFieldShape::CFieldShape( void ) :
    m_nScale( 1.0 )
{
}

CFieldShape::~CFieldShape( void )
{
}

BOOL CFieldShape::PrepareShape( void )
{
    if ( ! ( m_nScale > 0.0 ) )
        return RayJson::Fail( "scale must be more than 0" );
    return PrepareField();
}

double CFieldShape::FieldAt( const double anPoint[ 3 ] ) const
{
    double anUnit[ 3 ] = { anPoint[ 0 ] / m_nScale, anPoint[ 1 ] / m_nScale, anPoint[ 2 ] / m_nScale };

    return Field( anUnit ) * m_nScale;
}

BOOL CFieldShape::LocalHit( const FLT anOrigin[ 3 ], const FLT anDirection[ 3 ], FLT nLow, FLT nHigh, FLT &nT )
{
        // At size 1 the direction is the same unit vector and distances
        // shrink by the scale.
    double anO[ 3 ] = { anOrigin[ 0 ] / m_nScale, anOrigin[ 1 ] / m_nScale, anOrigin[ 2 ] / m_nScale };
    double anD[ 3 ] = { anDirection[ 0 ], anDirection[ 1 ], anDirection[ 2 ] };
    double nUnitT;

    if ( ! FieldHit( anO, anD, nLow / m_nScale, nHigh / m_nScale, nUnitT ) )
        return FALSE;
    nT = nUnitT * m_nScale;
    return ( nT > nLow && nT < nHigh ) ? TRUE : FALSE;
}

void CFieldShape::LocalNormal( const FLT anPoint[ 3 ], FLT anNormal[ 3 ] )
{
    double anUnit[ 3 ] = { anPoint[ 0 ] / m_nScale, anPoint[ 1 ] / m_nScale, anPoint[ 2 ] / m_nScale };
    double anOut[ 3 ];

    FieldNormal( anUnit, anOut );
    if ( ! ( ( anOut[ 0 ] * anOut[ 0 ] ) + ( anOut[ 1 ] * anOut[ 1 ] ) + ( anOut[ 2 ] * anOut[ 2 ] ) > 0.0 ) )
    {
        anOut[ 0 ] = 0.0;
        anOut[ 1 ] = 1.0;
        anOut[ 2 ] = 0.0;
    }
    for ( int i = 0; i < 3; i++ )
        anNormal[ i ] = anOut[ i ];
}

void CFieldShape::LocalCoords( const FLT anPoint[ 3 ], FLT &nU, FLT &nV )
{
    double nR = sqrt( ( anPoint[ 0 ] * anPoint[ 0 ] ) + ( anPoint[ 1 ] * anPoint[ 1 ] ) + ( anPoint[ 2 ] * anPoint[ 2 ] ) );

    if ( ! ( nR > 0.0 ) )
    {
        nU = 0.5;
        nV = 0.5;
        return;
    }

    double nUp = anPoint[ 1 ] / nR;

    nUp = ( nUp > 1.0 ) ? 1.0 : ( nUp < -1.0 ) ? -1.0 : nUp;
    nU = 0.5 + ( atan2( (double)anPoint[ 2 ], (double)anPoint[ 0 ] ) / ( 2.0 * 3.14159265358979323846 ) );
    nV = 0.5 + ( asin( nUp ) / 3.14159265358979323846 );
}

void CFieldShape::LocalBounds( FLT anLow[ 3 ], FLT anHigh[ 3 ] )
{
    double anL[ 3 ], anH[ 3 ];

    FieldBounds( anL, anH );
    for ( int i = 0; i < 3; i++ )
    {
        anLow[ i ] = anL[ i ] * m_nScale;
        anHigh[ i ] = anH[ i ] * m_nScale;
    }
}

void CFieldShape::ShapeToJson( IASLib::CObjectNode &oNode )
{
    FieldToJson( oNode );
    if ( m_nScale != 1.0 )
        RayJson::PutNumber( oNode, "scale", m_nScale );
}

BOOL CFieldShape::ShapeFromJson( const IASLib::CJsonNode &oNode )
{
    m_nScale = 1.0;
    return RayJson::GetNumber( oNode, "scale", m_nScale, FALSE ) && FieldFromJson( oNode );
}

BOOL CFieldShape::ClipToBox( const double anOrigin[ 3 ], const double anDirection[ 3 ], const double anLow[ 3 ],
                             const double anHigh[ 3 ], double &nLow, double &nHigh )
{
    for ( int i = 0; i < 3; i++ )
    {
        if ( anDirection[ i ] == 0.0 )
        {
            if ( anOrigin[ i ] < anLow[ i ] || anOrigin[ i ] > anHigh[ i ] )
                return FALSE;
            continue;
        }

        double t1 = ( anLow[ i ] - anOrigin[ i ] ) / anDirection[ i ];
        double t2 = ( anHigh[ i ] - anOrigin[ i ] ) / anDirection[ i ];

        if ( t1 > t2 )
        {
            double t = t1;

            t1 = t2;
            t2 = t;
        }
        nLow = ( t1 > nLow ) ? t1 : nLow;
        nHigh = ( t2 < nHigh ) ? t2 : nHigh;
        if ( nLow > nHigh )
            return FALSE;
    }
    return TRUE;
}

void CFieldShape::FieldGradient( const double anPoint[ 3 ], double nStep, double anGradient[ 3 ] ) const
{
    for ( int i = 0; i < 3; i++ )
    {
        double anA[ 3 ] = { anPoint[ 0 ], anPoint[ 1 ], anPoint[ 2 ] };
        double anB[ 3 ] = { anPoint[ 0 ], anPoint[ 1 ], anPoint[ 2 ] };

        anA[ i ] += nStep;
        anB[ i ] -= nStep;
        anGradient[ i ] = ( Field( anA ) - Field( anB ) ) / ( 2.0 * nStep );
    }
}

namespace
{
        // Where the field crosses zero along the edge from a to b.
    void Crossing( const double a[ 3 ], double fa, const double b[ 3 ], double fb, double anOut[ 3 ] )
    {
        double s = ( fa == fb ) ? 0.5 : fa / ( fa - fb );

        for ( int i = 0; i < 3; i++ )
            anOut[ i ] = a[ i ] + ( s * ( b[ i ] - a[ i ] ) );
    }
}

void CFieldShape::LocalTriangles( int nSegments, std::vector<FLT> &anPositions, std::vector<FLT> &anNormals )
{
        // Marching tetrahedra: the bounds cut into cubes, each cube into six
        // tetrahedra around its long diagonal, each tetrahedron giving the
        // triangles where the field changes sign across it.
    static const int s_anCorner[ 8 ][ 3 ] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 },
                                              { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } };
    static const int s_anTetra[ 6 ][ 4 ] = { { 0, 5, 1, 6 }, { 0, 1, 2, 6 }, { 0, 2, 3, 6 },
                                             { 0, 3, 7, 6 }, { 0, 7, 4, 6 }, { 0, 4, 5, 6 } };
    int    nCells = nSegments / 2;
    double anLow[ 3 ], anHigh[ 3 ], anCell[ 3 ];

    nCells = ( nCells < 8 ) ? 8 : ( nCells > 40 ) ? 40 : nCells;
    FieldBounds( anLow, anHigh );
    for ( int i = 0; i < 3; i++ )
        anCell[ i ] = ( anHigh[ i ] - anLow[ i ] ) / nCells;

        // The field at every grid point, worked out once.
    int                 nSide = nCells + 1;
    std::vector<double> anValues( (size_t)nSide * nSide * nSide );

    for ( int k = 0; k < nSide; k++ )
        for ( int j = 0; j < nSide; j++ )
            for ( int i = 0; i < nSide; i++ )
            {
                double p[ 3 ] = { anLow[ 0 ] + ( i * anCell[ 0 ] ), anLow[ 1 ] + ( j * anCell[ 1 ] ), anLow[ 2 ] + ( k * anCell[ 2 ] ) };

                anValues[ ( ( (size_t)k * nSide ) + j ) * nSide + i ] = Field( p );
            }

    double nStep = ( anCell[ 0 ] + anCell[ 1 ] + anCell[ 2 ] ) / 6.0;

    for ( int k = 0; k < nCells; k++ )
        for ( int j = 0; j < nCells; j++ )
            for ( int i = 0; i < nCells; i++ )
            {
                double aanP[ 8 ][ 3 ], anF[ 8 ];

                for ( int c = 0; c < 8; c++ )
                {
                    int ci = i + s_anCorner[ c ][ 0 ], cj = j + s_anCorner[ c ][ 1 ], ck = k + s_anCorner[ c ][ 2 ];

                    aanP[ c ][ 0 ] = anLow[ 0 ] + ( ci * anCell[ 0 ] );
                    aanP[ c ][ 1 ] = anLow[ 1 ] + ( cj * anCell[ 1 ] );
                    aanP[ c ][ 2 ] = anLow[ 2 ] + ( ck * anCell[ 2 ] );
                    anF[ c ] = anValues[ ( ( (size_t)ck * nSide ) + cj ) * nSide + ci ];
                }
                for ( int t = 0; t < 6; t++ )
                {
                    const int *v = s_anTetra[ t ];
                    int        anIn[ 4 ], anOut[ 4 ], nIn = 0, nOut = 0;

                    for ( int n = 0; n < 4; n++ )
                    {
                        if ( anF[ v[ n ] ] < 0.0 )
                            anIn[ nIn++ ] = v[ n ];
                        else
                            anOut[ nOut++ ] = v[ n ];
                    }
                    if ( nIn == 0 || nOut == 0 )
                        continue;

                    double aanQ[ 4 ][ 3 ];
                    int    nQ = 0;

                    if ( nIn == 1 || nOut == 1 )
                    {
                        int  nAlone = ( nIn == 1 ) ? anIn[ 0 ] : anOut[ 0 ];
                        int *anOthers = ( nIn == 1 ) ? anOut : anIn;

                        for ( int n = 0; n < 3; n++ )
                            Crossing( aanP[ nAlone ], anF[ nAlone ], aanP[ anOthers[ n ] ], anF[ anOthers[ n ] ], aanQ[ nQ++ ] );
                    }
                    else
                    {
                            // Two in, two out: a quad, its corners in order
                            // around it.
                        Crossing( aanP[ anIn[ 0 ] ], anF[ anIn[ 0 ] ], aanP[ anOut[ 0 ] ], anF[ anOut[ 0 ] ], aanQ[ nQ++ ] );
                        Crossing( aanP[ anIn[ 0 ] ], anF[ anIn[ 0 ] ], aanP[ anOut[ 1 ] ], anF[ anOut[ 1 ] ], aanQ[ nQ++ ] );
                        Crossing( aanP[ anIn[ 1 ] ], anF[ anIn[ 1 ] ], aanP[ anOut[ 1 ] ], anF[ anOut[ 1 ] ], aanQ[ nQ++ ] );
                        Crossing( aanP[ anIn[ 1 ] ], anF[ anIn[ 1 ] ], aanP[ anOut[ 0 ] ], anF[ anOut[ 0 ] ], aanQ[ nQ++ ] );
                    }
                    for ( int nTri = 0; nTri < nQ - 2; nTri++ )
                    {
                        const double *a = aanQ[ 0 ], *b = aanQ[ nTri + 1 ], *c = aanQ[ nTri + 2 ];
                        double        anNa[ 3 ], anNb[ 3 ], anNc[ 3 ];

                        FieldGradient( a, nStep, anNa );
                        FieldGradient( b, nStep, anNb );
                        FieldGradient( c, nStep, anNc );

                            // Wound so the face looks along the gradient.
                        double e1[ 3 ] = { b[ 0 ] - a[ 0 ], b[ 1 ] - a[ 1 ], b[ 2 ] - a[ 2 ] };
                        double e2[ 3 ] = { c[ 0 ] - a[ 0 ], c[ 1 ] - a[ 1 ], c[ 2 ] - a[ 2 ] };
                        double anFace[ 3 ] = { ( e1[ 1 ] * e2[ 2 ] ) - ( e1[ 2 ] * e2[ 1 ] ), ( e1[ 2 ] * e2[ 0 ] ) - ( e1[ 0 ] * e2[ 2 ] ),
                                               ( e1[ 0 ] * e2[ 1 ] ) - ( e1[ 1 ] * e2[ 0 ] ) };
                        double nFacing = ( anFace[ 0 ] * ( anNa[ 0 ] + anNb[ 0 ] + anNc[ 0 ] ) ) +
                                         ( anFace[ 1 ] * ( anNa[ 1 ] + anNb[ 1 ] + anNc[ 1 ] ) ) +
                                         ( anFace[ 2 ] * ( anNa[ 2 ] + anNb[ 2 ] + anNc[ 2 ] ) );
                        FLT           p[ 3 ][ 3 ], n[ 3 ][ 3 ];

                        for ( int i2 = 0; i2 < 3; i2++ )
                        {
                            p[ 0 ][ i2 ] = a[ i2 ] * m_nScale;
                            p[ 1 ][ i2 ] = b[ i2 ] * m_nScale;
                            p[ 2 ][ i2 ] = c[ i2 ] * m_nScale;
                            n[ 0 ][ i2 ] = anNa[ i2 ];
                            n[ 1 ][ i2 ] = anNb[ i2 ];
                            n[ 2 ][ i2 ] = anNc[ i2 ];
                        }
                        if ( nFacing >= 0.0 )
                            LocalMesh::Add( anPositions, anNormals, p[ 0 ], p[ 1 ], p[ 2 ], n[ 0 ], n[ 1 ], n[ 2 ] );
                        else
                            LocalMesh::Add( anPositions, anNormals, p[ 0 ], p[ 2 ], p[ 1 ], n[ 0 ], n[ 2 ], n[ 1 ] );
                    }
                }
            }
}
