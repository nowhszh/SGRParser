//
// Created by marvin on 23-7-18.
//

#pragma once

#include "AnsiColor.h"

#include <map>
#include <string>

namespace ANSI {

class ColorTable;

class SGRParseContext {
    friend class ColorTable;

  public:
    enum ColorVersion : uint8_t {
        BIT_8  = 5,
        BIT_24 = 2,
    };

    enum ReturnVal {
        RETURN_SUCCESS_BREAK,
        RETURN_SUCCESS_CONTINUE,

        RETURN_ERROR_BREAK,
        RETURN_ERROR_CONTINUE,
    };

    enum ParseResult {
        //        RESULT_CODE_ERROR = -1,
        RESULT_UNKNOWN_COLOR,
        RESULT_FRONT_COLOR,
        RESULT_BACK_COLOR,
        RESULT_DEFAULT_COLOR,
        RESULT_CURRENT_COLOR,
    };

    enum ParseState {
        //        STATE_HAS_RESULT,

        STATE_WAIT_FIRST_PARAMETER,
        STATE_WAIT_VERSION,
        STATE_WAIT_BIT_8_ARGS,
        STATE_WAIT_BIT_24_ARGS_R,
        STATE_WAIT_BIT_24_ARGS_G,
        STATE_WAIT_BIT_24_ARGS_B,
    };

  public:
    explicit SGRParseContext();

    ReturnVal parse( std::string_view& seqs );

    ReturnVal setFirstParameter( const std::string_view& num );
    ReturnVal setColorVersion( const std::string_view& num );
    ReturnVal setBit8Color( const std::string_view& num );
    ReturnVal setBit24Color( const std::string_view& num );

    inline void reset()
    {
        new ( this ) SGRParseContext();
    }

    inline ParseState state()
    {
        return state_;
    }

    inline ParseResult result()
    {
        return result_;
    }

    inline RGB rgb()
    {
        return color_;
    }

  private:
    SGRParseContext( ParseResult result, RGB rgb, ParseState s = STATE_WAIT_FIRST_PARAMETER );

    static ReturnVal stringToParameter( const std::string_view& num, uint8_t& val );

  private:
    ParseResult result_;
    ParseState  state_;
    RGB         color_;
    bool        bit24Valid_;
};

class ColorTable {
  public:
    enum ColorIndex : uint8_t {
        RESET_DEFAULT = 0,

        // 3/4-bit front color
        F_BLACK   = 30,
        F_RED     = 31,
        F_GREEN   = 32,
        F_YELLOW  = 33,
        F_BLUE    = 34,
        F_MAGENTA = 35,
        F_CYAN    = 36,
        F_WHITE   = 37,

        // custom front color
        F_CUSTOM_COLOR = 38,

        // 3/4-bit back color
        B_BLACK   = 40,
        B_RED     = 41,
        B_GREEN   = 42,
        B_YELLOW  = 43,
        B_BLUE    = 44,
        B_MAGENTA = 45,
        B_CYAN    = 46,
        B_WHITE   = 47,

        // custom back color
        B_CUSTOM_COLOR = 48,

        // 3/4-bit front bright color
        F_BRIGHT_BLACK   = 90,
        F_BRIGHT_RED     = 91,
        F_BRIGHT_GREEN   = 92,
        F_BRIGHT_YELLOW  = 93,
        F_BRIGHT_BLUE    = 94,
        F_BRIGHT_MAGENTA = 95,
        F_BRIGHT_CYAN    = 96,
        F_BRIGHT_WHITE   = 97,

        // 3/4-bit back bright color
        B_BRIGHT_BLACK   = 100,
        B_BRIGHT_RED     = 101,
        B_BRIGHT_GREEN   = 102,
        B_BRIGHT_YELLOW  = 103,
        B_BRIGHT_BLUE    = 104,
        B_BRIGHT_MAGENTA = 105,
        B_BRIGHT_CYAN    = 106,
        B_BRIGHT_WHITE   = 107,
    };

  private:
    static std::map<ColorIndex, SGRParseContext> colorTable;

  public:
    static SGRParseContext index( ColorIndex num )
    {
        auto ret = colorTable.find( num );
        if ( ret == colorTable.end() ) {
            return { SGRParseContext::ParseResult::RESULT_UNKNOWN_COLOR, {},
                SGRParseContext::STATE_WAIT_FIRST_PARAMETER };
        }
        return ret->second;
    }
};

} // namespace ANSI
