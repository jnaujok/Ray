#include "Globals.h"

#include "ClipColorModel.h"
#include "Color.h"

void CClipColorModel::ToRGB( const CColor &oColor, BYTE &nRed, BYTE &nGreen, BYTE &nBlue ) const
{
    CColor  oClipped( oColor );

        // Truncated rather than rounded, as the 1997 image writer did, so
        // images come out exactly as they did then. Clamp() also catches the
        // negative channels the 1997 code never guarded against.
    nRed = (BYTE)( Clamp( oClipped.GetR() ) * 255 );
    nGreen = (BYTE)( Clamp( oClipped.GetG() ) * 255 );
    nBlue = (BYTE)( Clamp( oClipped.GetB() ) * 255 );
}
