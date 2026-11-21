/*************************************************************************
*   CScalarMap, CColorMap                                                *
*                                                                        *
*   A setting that is either a constant or a texture read at a point:    *
*   every number and color of a material (and a light's tint and         *
*   intensity) can be either, so a texture can drive how see-through,    *
*   rough, refractive or reflective a surface is from place to place.    *
*                                                                        *
*   In a scene file a setting is written as before, a number or a        *
*   [red, green, blue] array, or as a texture:                           *
*                                                                        *
*       "roughness": { "type": "marble", ... }       any texture or preset*
*       "roughness": { "texture": { ... },           with these optional: *
*                      "scale": 0.5,                 value = offset +     *
*                      "offset": 0.1,                  scale x the texture*
*                      "channel": "red" }            which of its colors *
*                                                                        *
*   A number reads its texture's "average" of red, green and blue (the   *
*   default), "luminance", or one "red", "green" or "blue" channel. A    *
*   color reads the texture's color, times scale, plus offset. Values a  *
*   texture gives are kept within the setting's range (roughness 0 to 1, *
*   say); a constant is checked when it is read, as before.              *
*                                                                        *
*   A constant is saved exactly as before, so files that don't use maps  *
*   don't change. Copies share the texture.                              *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/
#ifndef __MAPS_H__
#define __MAPS_H__

#include "Globals.h"
#include "Color.h"
#include "Texture.h"
#include "Vector.h"

#include <memory>

class CObjects;

namespace IASLib
{
    class CJsonNode;
    class CObjectNode;
}

    // A point a map is read at, worked out the first time a map needs it,
    // so settings that are constants cost nothing.
class CMapPoint
{
    protected:
        CVector             m_oPoint;
        CObjects           *m_pObject;
        STexturePoint       m_oAt;
        BOOL                m_bDone;

    public:
            /// A point on (or inside) pObject; with no object, the scene point.
                            CMapPoint( const CVector &oPoint, CObjects *pObject );
            /// A point in space that isn't on an object's surface (a point a
            /// light falls on, a point inside glass): looked up in pObject's
            /// texture space if there is one, with no surface coordinates.
        static CMapPoint    InSpace( const CVector &oPoint, const CObjects *pObject );

        const STexturePoint &Get( void );
};

class CMapBase
{
    public:
        enum EChannel { CHANNEL_AVERAGE = 0, CHANNEL_LUMINANCE, CHANNEL_RED, CHANNEL_GREEN, CHANNEL_BLUE };

    protected:
        std::shared_ptr<const CTexture> m_pTexture;
        FLT                 m_nScale;
        FLT                 m_nOffset;
        int                 m_nChannel;
        BOOL                m_bWrapped;     // given as { "texture": ... }, so saved that way

                            CMapBase( void );

            // Reads a map from oValue (an object: a texture, or one with a
            // "texture"); bScalar allows "channel".
        BOOL                ReadMap( const IASLib::CJsonNode &oValue, BOOL bScalar );
        void                WriteMap( IASLib::CObjectNode &oParent, const char *strKey, BOOL bScalar ) const;
        CColor              TextureColor( const STexturePoint &oAt ) const;

    public:
            /// Whether a texture drives it (else it is its constant).
        BOOL                IsMapped( void ) const { return m_pTexture ? TRUE : FALSE; }
        const CTexture     *GetTexture( void ) const { return m_pTexture.get(); }
};

class CScalarMap : public CMapBase
{
    protected:
        FLT                 m_nValue;
        FLT                 m_nMin;
        FLT                 m_nMax;

    public:
                            CScalarMap( FLT nValue = 0.0 );

            /// Sets the constant (and drops any texture).
        CScalarMap         &operator =( FLT nValue );
            /// The constant (what it is where no texture drives it).
                            operator FLT( void ) const { return m_nValue; }
        FLT                 Get( void ) const { return m_nValue; }

            /// Keeps the values a texture gives within nMin to nMax.
        void                SetRange( FLT nMin, FLT nMax ) { m_nMin = nMin; m_nMax = nMax; }

            /// The value at a point: the constant, or the texture there.
        FLT                 At( const STexturePoint &oAt ) const;
        FLT                 At( CMapPoint &oAt ) const { return m_pTexture ? At( oAt.Get() ) : m_nValue; }

            /// Reads oNode[ strKey ]: a number or a texture. Missing, it keeps
            /// its value (or fails, if bRequired). FALSE, with RayJson::Fail,
            /// if it can't be read.
        BOOL                Read( const IASLib::CJsonNode &oNode, const char *strKey, BOOL bRequired );
            /// Writes it as oNode[ strKey ]: the number, or the texture.
        void                Write( IASLib::CObjectNode &oNode, const char *strKey ) const;
};

class CColorMap : public CMapBase
{
    protected:
        CColor              m_oValue;

    public:
                            CColorMap( void ) {}
                            CColorMap( const CColor &oValue ) : m_oValue( oValue ) {}
                            CColorMap( FLT nRed, FLT nGreen, FLT nBlue ) : m_oValue( nRed, nGreen, nBlue ) {}

        CColorMap          &operator =( const CColor &oValue );
                            operator const CColor &( void ) const { return m_oValue; }
        const CColor       &Get( void ) const { return m_oValue; }

        CColor              At( const STexturePoint &oAt ) const;
        CColor              At( CMapPoint &oAt ) const { return m_pTexture ? At( oAt.Get() ) : m_oValue; }

            /// Reads oNode[ strKey ]: a [red, green, blue] array or a texture.
        BOOL                Read( const IASLib::CJsonNode &oNode, const char *strKey, BOOL bRequired );
        void                Write( IASLib::CObjectNode &oNode, const char *strKey ) const;
};

#endif // __MAPS_H__
