/*************************************************************************
*   CExpression                                                          *
*                                                                        *
*   A formula in x, y and z. See Expression.h.                           *
*                                                                        *
* Copyright (c) 2026, Jeffrey R. Naujok                                  *
*************************************************************************/

    #include "Globals.h"

#include "Expression.h"

#include <math.h>
#include <stdlib.h>


namespace
{
    enum EOp
    {
        OP_CONST, OP_X, OP_Y, OP_Z, OP_NEG, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_POW,
        OP_SIN, OP_COS, OP_TAN, OP_ASIN, OP_ACOS, OP_ATAN, OP_ATAN2, OP_SQRT, OP_ABS, OP_EXP, OP_LOG,
        OP_MIN, OP_MAX, OP_FLOOR, OP_CLAMP, OP_MIX, OP_SMOOTHSTEP
    };

    struct SFunction
    {
        const char         *strName;
        int                 nOp;
        int                 nArguments;
    };

    const SFunction s_aoFunctions[] =
    {
        { "sin", OP_SIN, 1 }, { "cos", OP_COS, 1 }, { "tan", OP_TAN, 1 }, { "asin", OP_ASIN, 1 },
        { "acos", OP_ACOS, 1 }, { "atan", OP_ATAN, 1 }, { "atan2", OP_ATAN2, 2 }, { "sqrt", OP_SQRT, 1 },
        { "abs", OP_ABS, 1 }, { "exp", OP_EXP, 1 }, { "log", OP_LOG, 1 }, { "pow", OP_POW, 2 },
        { "min", OP_MIN, 2 }, { "max", OP_MAX, 2 }, { "floor", OP_FLOOR, 1 }, { "clamp", OP_CLAMP, 3 },
        { "mix", OP_MIX, 3 }, { "smoothstep", OP_SMOOTHSTEP, 3 }
    };

    const SFunction *FindFunction( const std::string &strName )
    {
        for ( size_t n = 0; n < sizeof( s_aoFunctions ) / sizeof( s_aoFunctions[ 0 ] ); n++ )
        {
            if ( strName == s_aoFunctions[ n ].strName )
                return &s_aoFunctions[ n ];
        }
        return NULL;
    }

        // How each op changes the stack's depth.
    int DepthChange( int nOp )
    {
        switch ( nOp )
        {
            case OP_CONST: case OP_X: case OP_Y: case OP_Z:
                return 1;
            case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_POW: case OP_ATAN2: case OP_MIN: case OP_MAX:
                return -1;
            case OP_CLAMP: case OP_MIX: case OP_SMOOTHSTEP:
                return -2;
            default:
                return 0;
        }
    }

        // A recursive-descent parser that writes the program as it goes, in
        // the order a stack machine runs it.
    class CParser
    {
        public:
            CParser( const std::string &strText, const std::map<std::string, double> &aoParameters,
                     std::vector<CExpression::SOp> &aoProgram ) :
                m_strText( strText ), m_aoParameters( aoParameters ), m_aoProgram( aoProgram ), m_nAt( 0 ) {}

            bool Parse( std::string &strError )
            {
                if ( ! Sum() )
                {
                    strError = m_strError;
                    return false;
                }
                Space();
                if ( m_nAt < m_strText.size() )
                {
                    strError = Where( m_nAt ) + "unexpected \"" + m_strText.substr( m_nAt, 1 ) + "\"";
                    return false;
                }
                return true;
            }

        private:
            const std::string                       &m_strText;
            const std::map<std::string, double>     &m_aoParameters;
            std::vector<CExpression::SOp>           &m_aoProgram;
            size_t                                  m_nAt;
            std::string                             m_strError;

            std::string Where( size_t nAt )
            {
                char achColumn[ 32 ];

                snprintf( achColumn, sizeof( achColumn ), "column %d: ", (int)nAt + 1 );
                return achColumn;
            }

            bool Fail( size_t nAt, const std::string &strWhat )
            {
                if ( m_strError.empty() )
                    m_strError = Where( nAt ) + strWhat;
                return false;
            }

            void Emit( int nOp, double nValue = 0.0 )
            {
                CExpression::SOp oOp;

                oOp.nCode = nOp;
                oOp.nValue = nValue;
                m_aoProgram.push_back( oOp );
            }

            void Space( void )
            {
                while ( m_nAt < m_strText.size() && ( m_strText[ m_nAt ] == ' ' || m_strText[ m_nAt ] == '\t' ||
                                                      m_strText[ m_nAt ] == '\n' || m_strText[ m_nAt ] == '\r' ) )
                    m_nAt++;
            }

            bool Next( char ch )
            {
                Space();
                if ( m_nAt < m_strText.size() && m_strText[ m_nAt ] == ch )
                {
                    m_nAt++;
                    return true;
                }
                return false;
            }

                // sum := product { ( + | - ) product }
            bool Sum( void )
            {
                if ( ! Product() )
                    return false;
                for ( ;; )
                {
                    if ( Next( '+' ) )
                    {
                        if ( ! Product() )
                            return false;
                        Emit( OP_ADD );
                    }
                    else if ( Next( '-' ) )
                    {
                        if ( ! Product() )
                            return false;
                        Emit( OP_SUB );
                    }
                    else
                        return true;
                }
            }

                // product := unary { ( * | / ) unary }
            bool Product( void )
            {
                if ( ! Unary() )
                    return false;
                for ( ;; )
                {
                    if ( Next( '*' ) )
                    {
                        if ( ! Unary() )
                            return false;
                        Emit( OP_MUL );
                    }
                    else if ( Next( '/' ) )
                    {
                        if ( ! Unary() )
                            return false;
                        Emit( OP_DIV );
                    }
                    else
                        return true;
                }
            }

                // unary := ( - | + ) unary | power
            bool Unary( void )
            {
                if ( Next( '-' ) )
                {
                    if ( ! Unary() )
                        return false;
                    Emit( OP_NEG );
                    return true;
                }
                if ( Next( '+' ) )
                    return Unary();
                return Power();
            }

                // power := atom [ ^ unary ], so 2^-1 and 2^3^2 (= 2^9) work.
            bool Power( void )
            {
                if ( ! Atom() )
                    return false;
                if ( Next( '^' ) )
                {
                    if ( ! Unary() )
                        return false;
                    Emit( OP_POW );
                }
                return true;
            }

                // atom := number | name | function ( arguments ) | ( sum )
            bool Atom( void )
            {
                Space();
                if ( m_nAt >= m_strText.size() )
                    return Fail( m_nAt, "the formula ends too soon" );

                char ch = m_strText[ m_nAt ];

                if ( ch == '(' )
                {
                    size_t nOpen = m_nAt++;

                    if ( ! Sum() )
                        return false;
                    if ( ! Next( ')' ) )
                        return Fail( nOpen, "this \"(\" isn't closed" );
                    return true;
                }
                if ( ( ch >= '0' && ch <= '9' ) || ch == '.' )
                {
                    const char *pStart = m_strText.c_str() + m_nAt;
                    char       *pEnd = NULL;
                    double      nValue = strtod( pStart, &pEnd );

                    if ( pEnd == pStart )
                        return Fail( m_nAt, "a number is expected" );
                    m_nAt += pEnd - pStart;
                    Emit( OP_CONST, nValue );
                    return true;
                }
                if ( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) || ch == '_' )
                {
                    size_t nStart = m_nAt;

                    while ( m_nAt < m_strText.size() &&
                            ( ( m_strText[ m_nAt ] >= 'a' && m_strText[ m_nAt ] <= 'z' ) ||
                              ( m_strText[ m_nAt ] >= 'A' && m_strText[ m_nAt ] <= 'Z' ) ||
                              ( m_strText[ m_nAt ] >= '0' && m_strText[ m_nAt ] <= '9' ) || m_strText[ m_nAt ] == '_' ) )
                        m_nAt++;

                    std::string strName = m_strText.substr( nStart, m_nAt - nStart );
                    const SFunction *pFunction = FindFunction( strName );

                    if ( pFunction )
                    {
                        if ( ! Next( '(' ) )
                            return Fail( nStart, strName + " needs its arguments in parentheses" );
                        for ( int n = 0; n < pFunction->nArguments; n++ )
                        {
                            if ( n > 0 && ! Next( ',' ) )
                            {
                                char achCount[ 64 ];

                                snprintf( achCount, sizeof( achCount ), " takes %d arguments", pFunction->nArguments );
                                return Fail( nStart, strName + achCount );
                            }
                            if ( ! Sum() )
                                return false;
                        }
                        if ( ! Next( ')' ) )
                        {
                            char achCount[ 64 ];

                            snprintf( achCount, sizeof( achCount ), " takes %d argument%s", pFunction->nArguments,
                                      ( pFunction->nArguments == 1 ) ? "" : "s" );
                            return Fail( nStart, strName + achCount );
                        }
                        Emit( pFunction->nOp );
                        return true;
                    }
                    if ( strName == "x" )
                        Emit( OP_X );
                    else if ( strName == "y" )
                        Emit( OP_Y );
                    else if ( strName == "z" )
                        Emit( OP_Z );
                    else if ( strName == "pi" )
                        Emit( OP_CONST, 3.14159265358979323846 );
                    else if ( strName == "e" )
                        Emit( OP_CONST, 2.71828182845904523536 );
                    else
                    {
                        std::map<std::string, double>::const_iterator it = m_aoParameters.find( strName );

                        if ( it == m_aoParameters.end() )
                            return Fail( nStart, "unknown name \"" + strName + "\"" );
                        Emit( OP_CONST, it->second );
                    }
                    return true;
                }
                return Fail( m_nAt, "unexpected \"" + m_strText.substr( m_nAt, 1 ) + "\"" );
            }
    };

        // A number with its slopes along x, y and z, for exact gradients.
    struct SDual
    {
        double v, g[ 3 ];
    };

    inline SDual   Constant( double n, SDual * ) { SDual d = { n, { 0.0, 0.0, 0.0 } }; return d; }
    inline double  Constant( double n, double * ) { return n; }

        // f(a) with derivative f'(a).
    inline SDual Chain( const SDual &a, double nValue, double nSlope )
    {
        SDual d = { nValue, { nSlope * a.g[ 0 ], nSlope * a.g[ 1 ], nSlope * a.g[ 2 ] } };
        return d;
    }

    inline double Add( double a, double b ) { return a + b; }
    inline double Sub( double a, double b ) { return a - b; }
    inline double Mul( double a, double b ) { return a * b; }
    inline double Div( double a, double b ) { return a / b; }
    inline double Neg( double a ) { return -a; }
    inline double Pow( double a, double b ) { return pow( a, b ); }
    inline double Sin( double a ) { return sin( a ); }
    inline double Cos( double a ) { return cos( a ); }
    inline double Tan( double a ) { return tan( a ); }
    inline double Asin( double a ) { return asin( a ); }
    inline double Acos( double a ) { return acos( a ); }
    inline double Atan( double a ) { return atan( a ); }
    inline double Atan2( double a, double b ) { return atan2( a, b ); }
    inline double Sqrt( double a ) { return sqrt( a ); }
    inline double Abs( double a ) { return fabs( a ); }
    inline double Exp( double a ) { return exp( a ); }
    inline double Log( double a ) { return log( a ); }
    inline double Floor( double a ) { return floor( a ); }
    inline double Min( double a, double b ) { return ( b < a ) ? b : a; }
    inline double Max( double a, double b ) { return ( b > a ) ? b : a; }

    inline SDual Add( const SDual &a, const SDual &b )
    {
        SDual d = { a.v + b.v, { a.g[ 0 ] + b.g[ 0 ], a.g[ 1 ] + b.g[ 1 ], a.g[ 2 ] + b.g[ 2 ] } };
        return d;
    }
    inline SDual Sub( const SDual &a, const SDual &b )
    {
        SDual d = { a.v - b.v, { a.g[ 0 ] - b.g[ 0 ], a.g[ 1 ] - b.g[ 1 ], a.g[ 2 ] - b.g[ 2 ] } };
        return d;
    }
    inline SDual Mul( const SDual &a, const SDual &b )
    {
        SDual d = { a.v * b.v, { ( a.g[ 0 ] * b.v ) + ( a.v * b.g[ 0 ] ), ( a.g[ 1 ] * b.v ) + ( a.v * b.g[ 1 ] ),
                                 ( a.g[ 2 ] * b.v ) + ( a.v * b.g[ 2 ] ) } };
        return d;
    }
    inline SDual Div( const SDual &a, const SDual &b )
    {
        double nB2 = b.v * b.v;
        SDual  d = { a.v / b.v, { ( ( a.g[ 0 ] * b.v ) - ( a.v * b.g[ 0 ] ) ) / nB2, ( ( a.g[ 1 ] * b.v ) - ( a.v * b.g[ 1 ] ) ) / nB2,
                                  ( ( a.g[ 2 ] * b.v ) - ( a.v * b.g[ 2 ] ) ) / nB2 } };
        return d;
    }
    inline SDual Neg( const SDual &a ) { return Chain( a, -a.v, -1.0 ); }
    inline SDual Pow( const SDual &a, const SDual &b )
    {
        double nValue = pow( a.v, b.v );

            // A constant exponent: b a^(b-1), which also works for a <= 0.
        if ( b.g[ 0 ] == 0.0 && b.g[ 1 ] == 0.0 && b.g[ 2 ] == 0.0 )
            return Chain( a, nValue, ( b.v == 0.0 ) ? 0.0 : b.v * pow( a.v, b.v - 1.0 ) );

        double nLog = log( a.v );
        SDual  d;

        d.v = nValue;
        for ( int i = 0; i < 3; i++ )
            d.g[ i ] = nValue * ( ( b.g[ i ] * nLog ) + ( b.v * a.g[ i ] / a.v ) );
        return d;
    }
    inline SDual Sin( const SDual &a ) { return Chain( a, sin( a.v ), cos( a.v ) ); }
    inline SDual Cos( const SDual &a ) { return Chain( a, cos( a.v ), -sin( a.v ) ); }
    inline SDual Tan( const SDual &a ) { double c = cos( a.v ); return Chain( a, tan( a.v ), 1.0 / ( c * c ) ); }
    inline SDual Asin( const SDual &a ) { return Chain( a, asin( a.v ), 1.0 / sqrt( 1.0 - ( a.v * a.v ) ) ); }
    inline SDual Acos( const SDual &a ) { return Chain( a, acos( a.v ), -1.0 / sqrt( 1.0 - ( a.v * a.v ) ) ); }
    inline SDual Atan( const SDual &a ) { return Chain( a, atan( a.v ), 1.0 / ( 1.0 + ( a.v * a.v ) ) ); }
    inline SDual Atan2( const SDual &y, const SDual &x )
    {
        double nR2 = ( x.v * x.v ) + ( y.v * y.v );
        SDual  d;

        d.v = atan2( y.v, x.v );
        for ( int i = 0; i < 3; i++ )
            d.g[ i ] = ( nR2 > 0.0 ) ? ( ( x.v * y.g[ i ] ) - ( y.v * x.g[ i ] ) ) / nR2 : 0.0;
        return d;
    }
    inline SDual Sqrt( const SDual &a ) { double s = sqrt( a.v ); return Chain( a, s, ( s > 0.0 ) ? 0.5 / s : 0.0 ); }
    inline SDual Abs( const SDual &a ) { return Chain( a, fabs( a.v ), ( a.v < 0.0 ) ? -1.0 : 1.0 ); }
    inline SDual Exp( const SDual &a ) { double e = exp( a.v ); return Chain( a, e, e ); }
    inline SDual Log( const SDual &a ) { return Chain( a, log( a.v ), 1.0 / a.v ); }
    inline SDual Floor( const SDual &a ) { return Chain( a, floor( a.v ), 0.0 ); }
    inline SDual Min( const SDual &a, const SDual &b ) { return ( b.v < a.v ) ? b : a; }
    inline SDual Max( const SDual &a, const SDual &b ) { return ( b.v > a.v ) ? b : a; }

    template <typename T> T Clamp( const T &x, const T &lo, const T &hi ) { return Min( Max( x, lo ), hi ); }

    template <typename T> T Mix( const T &a, const T &b, const T &t ) { return Add( a, Mul( Sub( b, a ), t ) ); }

    template <typename T> T Smoothstep( const T &e0, const T &e1, const T &x )
    {
        T t = Clamp( Div( Sub( x, e0 ), Sub( e1, e0 ) ), Constant( 0.0, (T *)NULL ), Constant( 1.0, (T *)NULL ) );

        return Mul( Mul( t, t ), Sub( Constant( 3.0, (T *)NULL ), Mul( Constant( 2.0, (T *)NULL ), t ) ) );
    }

        // Runs the program on a stack of T (double, or SDual for slopes).
    template <typename T> T Run( const std::vector<CExpression::SOp> &aoProgram, const T &x, const T &y, const T &z )
    {
        T      aoStack[ CExpression::MAX_DEPTH ];
        int    nTop = 0;

        for ( size_t n = 0; n < aoProgram.size(); n++ )
        {
            const CExpression::SOp &oOp = aoProgram[ n ];

            switch ( oOp.nCode )
            {
                case OP_CONST:  aoStack[ nTop++ ] = Constant( oOp.nValue, (T *)NULL ); break;
                case OP_X:      aoStack[ nTop++ ] = x; break;
                case OP_Y:      aoStack[ nTop++ ] = y; break;
                case OP_Z:      aoStack[ nTop++ ] = z; break;
                case OP_NEG:    aoStack[ nTop - 1 ] = Neg( aoStack[ nTop - 1 ] ); break;
                case OP_ADD:    nTop--; aoStack[ nTop - 1 ] = Add( aoStack[ nTop - 1 ], aoStack[ nTop ] ); break;
                case OP_SUB:    nTop--; aoStack[ nTop - 1 ] = Sub( aoStack[ nTop - 1 ], aoStack[ nTop ] ); break;
                case OP_MUL:    nTop--; aoStack[ nTop - 1 ] = Mul( aoStack[ nTop - 1 ], aoStack[ nTop ] ); break;
                case OP_DIV:    nTop--; aoStack[ nTop - 1 ] = Div( aoStack[ nTop - 1 ], aoStack[ nTop ] ); break;
                case OP_POW:    nTop--; aoStack[ nTop - 1 ] = Pow( aoStack[ nTop - 1 ], aoStack[ nTop ] ); break;
                case OP_ATAN2:  nTop--; aoStack[ nTop - 1 ] = Atan2( aoStack[ nTop - 1 ], aoStack[ nTop ] ); break;
                case OP_MIN:    nTop--; aoStack[ nTop - 1 ] = Min( aoStack[ nTop - 1 ], aoStack[ nTop ] ); break;
                case OP_MAX:    nTop--; aoStack[ nTop - 1 ] = Max( aoStack[ nTop - 1 ], aoStack[ nTop ] ); break;
                case OP_SIN:    aoStack[ nTop - 1 ] = Sin( aoStack[ nTop - 1 ] ); break;
                case OP_COS:    aoStack[ nTop - 1 ] = Cos( aoStack[ nTop - 1 ] ); break;
                case OP_TAN:    aoStack[ nTop - 1 ] = Tan( aoStack[ nTop - 1 ] ); break;
                case OP_ASIN:   aoStack[ nTop - 1 ] = Asin( aoStack[ nTop - 1 ] ); break;
                case OP_ACOS:   aoStack[ nTop - 1 ] = Acos( aoStack[ nTop - 1 ] ); break;
                case OP_ATAN:   aoStack[ nTop - 1 ] = Atan( aoStack[ nTop - 1 ] ); break;
                case OP_SQRT:   aoStack[ nTop - 1 ] = Sqrt( aoStack[ nTop - 1 ] ); break;
                case OP_ABS:    aoStack[ nTop - 1 ] = Abs( aoStack[ nTop - 1 ] ); break;
                case OP_EXP:    aoStack[ nTop - 1 ] = Exp( aoStack[ nTop - 1 ] ); break;
                case OP_LOG:    aoStack[ nTop - 1 ] = Log( aoStack[ nTop - 1 ] ); break;
                case OP_FLOOR:  aoStack[ nTop - 1 ] = Floor( aoStack[ nTop - 1 ] ); break;
                case OP_CLAMP:
                    nTop -= 2;
                    aoStack[ nTop - 1 ] = Clamp( aoStack[ nTop - 1 ], aoStack[ nTop ], aoStack[ nTop + 1 ] );
                    break;
                case OP_MIX:
                    nTop -= 2;
                    aoStack[ nTop - 1 ] = Mix( aoStack[ nTop - 1 ], aoStack[ nTop ], aoStack[ nTop + 1 ] );
                    break;
                case OP_SMOOTHSTEP:
                    nTop -= 2;
                    aoStack[ nTop - 1 ] = Smoothstep( aoStack[ nTop - 1 ], aoStack[ nTop ], aoStack[ nTop + 1 ] );
                    break;
            }
        }
        return aoStack[ 0 ];
    }
}

CExpression::CExpression( void )
{
}

bool CExpression::IsReserved( const std::string &strName )
{
    return strName == "x" || strName == "y" || strName == "z" || strName == "pi" || strName == "e" || FindFunction( strName ) != NULL;
}

bool CExpression::Compile( const std::string &strText, const std::map<std::string, double> &aoParameters, std::string &strError )
{
    std::vector<SOp> aoProgram;
    CParser          oParser( strText, aoParameters, aoProgram );

    m_aoProgram.clear();
    if ( ! oParser.Parse( strError ) )
        return false;

        // The stack's depth, which the evaluator's fixed stack must hold.
    int nDepth = 0;

    for ( size_t n = 0; n < aoProgram.size(); n++ )
    {
        nDepth += DepthChange( aoProgram[ n ].nCode );
        if ( nDepth > MAX_DEPTH )
        {
            strError = "column 1: the formula nests too deeply";
            return false;
        }
    }
    m_aoProgram = aoProgram;
    return true;
}

double CExpression::Value( double x, double y, double z ) const
{
    if ( m_aoProgram.empty() )
        return 0.0;
    return Run<double>( m_aoProgram, x, y, z );
}

double CExpression::Gradient( double x, double y, double z, double anGradient[ 3 ] ) const
{
    if ( m_aoProgram.empty() )
    {
        anGradient[ 0 ] = anGradient[ 1 ] = anGradient[ 2 ] = 0.0;
        return 0.0;
    }

    SDual dx = { x, { 1.0, 0.0, 0.0 } };
    SDual dy = { y, { 0.0, 1.0, 0.0 } };
    SDual dz = { z, { 0.0, 0.0, 1.0 } };
    SDual d = Run<SDual>( m_aoProgram, dx, dy, dz );

    for ( int i = 0; i < 3; i++ )
        anGradient[ i ] = d.g[ i ];
    return d.v;
}
