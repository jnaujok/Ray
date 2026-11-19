/*************************************************************************
*   CColor header                                                        *
*                                                                        *
* Defines the CColor class for use in the ray-tracer.                    *
*                                                                        *
* Copyright (c) 1996, Jeffrey R. Naujok                                  *
*************************************************************************/

#ifndef __COLOR_H__
#define __COLOR_H__

class CVector;
class CStream;

class CColor
{
    protected:
        FLT         m_nRed;
        FLT         m_nGreen;
        FLT         m_nBlue;
   public:
                    CColor( FLT nRed = 0.00, FLT nGreen = 0.00, FLT nBlue = 0.00 );
                    CColor( CVector &oSource );
                    CColor( const CColor &oSource );
        virtual    ~CColor( void );
      
        FLT         Distance( const CColor &oCompare );
        CColor      operator +( const CColor &oAdd );
        CColor      operator *( FLT nScale );
        CColor      operator *( const CColor &oMult );
        void        Clip( void );
        CColor     &operator =( const CColor &oSource );
        CColor     &operator =( CVector &oSource );
        BOOL        operator ==( const CColor &oCompare );
        BOOL        operator !=( const CColor &oCompare );
        FLT         GetR( void ) {return m_nRed; }
        FLT         GetG( void ) {return m_nGreen; }
        FLT         GetB( void ) {return m_nBlue; }
        void        Set( FLT nRed, FLT nGreen, FLT nBlue, FLT nGamma = 1.0 );

        friend CStream &operator >>( CStream &oAr, CColor &oColor );
        friend CStream &operator <<( CStream &oAr, CColor &oColor );

#ifdef _INC_WINDOWS
            // Casting operator for windows to change this object to a Windows Color
            // reference automagically
                    operator COLORREF();
        CColor     &operator =( COLORREF crSource );
#endif
};

#endif // __COLOR_H__