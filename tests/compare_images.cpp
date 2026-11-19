// Compares two images in the ray tracer's raw format (2-byte width, 2-byte
// height, then RGB bytes).
//
//   compare_images <actual> <expected> [--exact]
//
// Without --exact, small differences from floating-point rounding on other
// compilers and processors are allowed: the images match when at most 1% of
// the pixels differ by more than 8 levels in any channel and the mean
// difference is under 1 level.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

static bool Load( const char *strFile, int &nWidth, int &nHeight, std::vector<unsigned char> &aPixels )
{
    FILE           *pFile = fopen( strFile, "rb" );
    unsigned char   achHeader[ 4 ];

    if ( pFile == NULL )
    {
        fprintf( stderr, "Can't open %s\n", strFile );
        return false;
    }

    bool bOk = fread( achHeader, 1, 4, pFile ) == 4;
    if ( bOk )
    {
        nWidth = achHeader[ 0 ] * 256 + achHeader[ 1 ];
        nHeight = achHeader[ 2 ] * 256 + achHeader[ 3 ];
        aPixels.resize( (size_t)nWidth * (size_t)nHeight * 3 );
        bOk = fread( aPixels.data(), 1, aPixels.size(), pFile ) == aPixels.size();
    }
    fclose( pFile );

    if ( ! bOk )
        fprintf( stderr, "%s is too short\n", strFile );
    return bOk;
}

int main( int argc, char **argv )
{
    int                         nWidthA, nHeightA, nWidthB, nHeightB;
    std::vector<unsigned char>  aActual, aExpected;
    bool                        bExact = ( argc > 3 ) && ( strcmp( argv[ 3 ], "--exact" ) == 0 );

    if ( argc < 3 )
    {
        fprintf( stderr, "usage: compare_images <actual> <expected> [--exact]\n" );
        return 2;
    }

    if ( ! Load( argv[ 1 ], nWidthA, nHeightA, aActual ) || ! Load( argv[ 2 ], nWidthB, nHeightB, aExpected ) )
        return 1;

    if ( nWidthA != nWidthB || nHeightA != nHeightB )
    {
        fprintf( stderr, "Sizes differ: %dx%d against %dx%d\n", nWidthA, nHeightA, nWidthB, nHeightB );
        return 1;
    }

    size_t  nPixels = (size_t)nWidthA * (size_t)nHeightA;
    size_t  nDifferent = 0;
    size_t  nBadPixels = 0;
    double  dTotal = 0.0;
    int     nWorst = 0;

    for ( size_t nPixel = 0; nPixel < nPixels; nPixel++ )
    {
        int nPixelWorst = 0;

        for ( int nChannel = 0; nChannel < 3; nChannel++ )
        {
            int nDiff = abs( (int)aActual[ nPixel * 3 + nChannel ] - (int)aExpected[ nPixel * 3 + nChannel ] );

            dTotal += nDiff;
            if ( nDiff > nPixelWorst )
                nPixelWorst = nDiff;
        }
        if ( nPixelWorst > 0 )
            nDifferent++;
        if ( nPixelWorst > 8 )
            nBadPixels++;
        if ( nPixelWorst > nWorst )
            nWorst = nPixelWorst;
    }

    double dMean = ( nPixels ) ? dTotal / (double)( nPixels * 3 ) : 0.0;

    printf( "%dx%d: %zu pixels differ (%zu by more than 8), worst %d, mean %.4f\n",
            nWidthA, nHeightA, nDifferent, nBadPixels, nWorst, dMean );

    if ( bExact )
        return ( nDifferent == 0 ) ? 0 : 1;

    bool bMatch = ( (double)nBadPixels <= (double)nPixels * 0.01 ) && ( dMean < 1.0 );
    return bMatch ? 0 : 1;
}
