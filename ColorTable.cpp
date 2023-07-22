//
// Created by marvin on 23-7-18.
//

#include "ColorTable.h"

#include "SGRParser.h"

namespace ANSI {

std::map<ColorTable::ColorE, SGRParseContext> ColorTable::colorTable {
    { ColorTable::RESET_DEFAULT,
        { SGRParseContext::FRONT_AND_BACK, {}, SGRParseContext::STATE_DEFAULT_COLOR } },

    // 7bit front color
    { ColorTable::F_BLACK, { SGRParseContext::FRONT_COLOR, { 1, 1, 1 } } },
    { ColorTable::F_RED, { SGRParseContext::FRONT_COLOR, { 222, 56, 43 } } },
    { ColorTable::F_GREEN, { SGRParseContext::FRONT_COLOR, { 57, 181, 74 } } },
    { ColorTable::F_YELLOW, { SGRParseContext::FRONT_COLOR, { 255, 199, 6 } } },
    { ColorTable::F_BLUE, { SGRParseContext::FRONT_COLOR, { 0, 111, 184 } } },
    { ColorTable::F_MAGENTA, { SGRParseContext::FRONT_COLOR, { 118, 38, 113 } } },
    { ColorTable::F_CYAN, { SGRParseContext::FRONT_COLOR, { 44, 181, 233 } } },
    { ColorTable::F_WHITE, { SGRParseContext::FRONT_COLOR, { 204, 204, 204 } } },

    // custom front color
    { ColorTable::F_CUSTOM_COLOR,
        { SGRParseContext::FRONT_COLOR, {}, SGRParseContext::STATE_WAIT_VERSION } },

    // 7bit back color
    { ColorTable::B_BLACK, { SGRParseContext::BACK_COLOR, { 1, 1, 1 } } },
    { ColorTable::B_RED, { SGRParseContext::BACK_COLOR, { 222, 56, 43 } } },
    { ColorTable::B_GREEN, { SGRParseContext::BACK_COLOR, { 57, 181, 74 } } },
    { ColorTable::B_YELLOW, { SGRParseContext::BACK_COLOR, { 255, 199, 6 } } },
    { ColorTable::B_BLUE, { SGRParseContext::BACK_COLOR, { 0, 111, 184 } } },
    { ColorTable::B_MAGENTA, { SGRParseContext::BACK_COLOR, { 118, 38, 113 } } },
    { ColorTable::B_CYAN, { SGRParseContext::BACK_COLOR, { 44, 181, 233 } } },
    { ColorTable::B_WHITE, { SGRParseContext::BACK_COLOR, { 204, 204, 204 } } },

    // custom back color
    { ColorTable::B_CUSTOM_COLOR,
        { SGRParseContext::BACK_COLOR, {}, SGRParseContext::STATE_WAIT_VERSION } },

    // 7bit front bright color
    { ColorTable::F_BRIGHT_BLACK, { SGRParseContext::FRONT_COLOR, { 128, 128, 128 } } },
    { ColorTable::F_BRIGHT_RED, { SGRParseContext::FRONT_COLOR, { 255, 0, 0 } } },
    { ColorTable::F_BRIGHT_GREEN, { SGRParseContext::FRONT_COLOR, { 0, 255, 0 } } },
    { ColorTable::F_BRIGHT_YELLOW, { SGRParseContext::FRONT_COLOR, { 255, 255, 0 } } },
    { ColorTable::F_BRIGHT_BLUE, { SGRParseContext::FRONT_COLOR, { 0, 0, 255 } } },
    { ColorTable::F_BRIGHT_MAGENTA, { SGRParseContext::FRONT_COLOR, { 255, 0, 255 } } },
    { ColorTable::F_BRIGHT_CYAN, { SGRParseContext::FRONT_COLOR, { 0, 255, 255 } } },
    { ColorTable::F_BRIGHT_WHITE, { SGRParseContext::FRONT_COLOR, { 255, 255, 255 } } },

    // 7bit back bright color
    { ColorTable::B_BRIGHT_BLACK, { SGRParseContext::BACK_COLOR, { 128, 128, 128 } } },
    { ColorTable::B_BRIGHT_RED, { SGRParseContext::BACK_COLOR, { 255, 0, 0 } } },
    { ColorTable::B_BRIGHT_GREEN, { SGRParseContext::BACK_COLOR, { 0, 255, 0 } } },
    { ColorTable::B_BRIGHT_YELLOW, { SGRParseContext::BACK_COLOR, { 255, 255, 0 } } },
    { ColorTable::B_BRIGHT_BLUE, { SGRParseContext::BACK_COLOR, { 0, 0, 255 } } },
    { ColorTable::B_BRIGHT_MAGENTA, { SGRParseContext::BACK_COLOR, { 255, 0, 255 } } },
    { ColorTable::B_BRIGHT_CYAN, { SGRParseContext::BACK_COLOR, { 0, 255, 255 } } },
    { ColorTable::B_BRIGHT_WHITE, { SGRParseContext::BACK_COLOR, { 255, 255, 255 } } },
};

std::pair<bool, uint8_t> base10ToU8( const std::string_view& num )
{
    bool has   = false;
    int  value = 0;

    for ( auto ch : num ) {
        if ( ch > '9' || ch < '0' ) {
            has = false;
            break;
        }

        value *= 10;
        value += ( ch - '0' );
        has = true;
    }

    if ( value > std::numeric_limits<uint8_t>::max() ) {
        // TODO: not u8, continue, difference convert error
        return { false, {} };
    }
    return { has, static_cast<uint8_t>( value ) };
}

SGRParseContext::SGRParseContext()
    : position_( ColorPosition::UNKNOWN )
    , state_( STATE_WAIT_FIRST_PARAMETER )
    , color_()
    , bit24Valid( true )
{
}

SGRParseContext::SGRParseContext( ColorPosition t, RGB rgb, ParseState s )
    : position_( t )
    , state_( s )
    , color_( rgb )
    , bit24Valid( true )
{
}

SGRParseContext::ReturnVal SGRParseContext::setFirstParameter( const std::string_view& num )
{
    // in first parameter, empty will default value is 0, 0 will use default color
    if ( num.empty() ) {
        state_ = STATE_DEFAULT_COLOR;
        return ReturnVal::SUCCESS;
    }
    // convert to number, error parameter will break parse, and keep current color
    auto [ ok, value ] = base10ToU8( num );
    if ( !ok ) {
        state_ = STATE_CURRENT_COLOR;
        return ReturnVal::ERROR_AND_BREAK;
    }

    *this = ColorTable::index( ColorTable::ColorE( value ) );
    // UNKNOWN is not support, so continue
    return (
        position_ == ColorPosition::UNKNOWN ? ReturnVal::ERROR_AND_CONTINUE : ReturnVal::SUCCESS );
}

SGRParseContext::ReturnVal SGRParseContext::setColorVersion( const std::string_view& num )
{
    // before version parameter is 38, empty parameter will reset parse state, and use last color
    if ( num.empty() ) {
        state_ = STATE_CURRENT_COLOR;
        return ReturnVal::ERROR_AND_CONTINUE;
    }

    // convert to number, error parameter will break parse, and keep current color
    auto [ ok, value ] = base10ToU8( num );
    if ( !ok ) {
        state_ = STATE_CURRENT_COLOR;
        return ReturnVal::ERROR_AND_BREAK;
    }

    // update state
    if ( BIT_8 == value ) {
        state_ = STATE_WAIT_BIT_8_ARGS;
    }
    else if ( BIT_24 == value ) {
        state_ = STATE_WAIT_BIT_24_ARGS_R;
    }
    else {
        // invalid value will reset parse state, and use last color
        state_ = STATE_CURRENT_COLOR;
        return ReturnVal::ERROR_AND_CONTINUE;
    }
    return ReturnVal::SUCCESS;
}

SGRParseContext::ReturnVal SGRParseContext::setBit8Color( const std::string_view& num )
{
    // 8-bit color parameter empty, will use last color, then parse continue
    if ( num.empty() ) {
        state_ = STATE_CURRENT_COLOR;
        return ReturnVal::ERROR_AND_CONTINUE;
    }

    // convert to number, error parameter will break parse, and keep current color
    auto [ ok, value ] = base10ToU8( num );
    if ( !ok ) {
        state_ = STATE_CURRENT_COLOR;
        return ReturnVal::ERROR_AND_BREAK;
    }

    // Standard colors
    if ( value >= 0 && value <= 7 ) {
        *this = ColorTable::index( ColorTable::ColorE( ColorTable::F_BLACK + value ) );
    }
    // High-intensity colors
    else if ( value >= 8 && value <= 15 ) {
        *this = ColorTable::index( ColorTable::ColorE( ColorTable::F_BRIGHT_BLACK + value ) );
    }
    // Grayscale colors
    else if ( value >= 232 && value <= 255 ) {
        auto colorValue = uint8_t( ( value - 232 ) * 10 + 8 );
        color_          = { colorValue, colorValue, colorValue };
        state_          = ParseState::STATE_SUCCESS;
    }
    // 216 colors
    else {
        constexpr uint8_t colorValue[] { 0, 95, 135, 175, 215, 255 };
        auto              val = value - 16;
        auto              x   = val / 36;
        val                   = val % 36;
        auto y                = val / 6;
        auto z                = val % 6;
        color_                = { colorValue[ x ], colorValue[ y ], colorValue[ z ] };
        state_                = ParseState::STATE_SUCCESS;
    }

    return ReturnVal::SUCCESS;
}

SGRParseContext::ReturnVal SGRParseContext::setBit24Color( const std::string_view& num )
{
    if ( num.empty() ) {
        // if bit24 color parameter empty, all the 24bit color parameters are invalid.
        bit24Valid = false;
    }

    decltype( color_.r )* colorVal;

    switch ( state_ ) {
    case ParseState::STATE_WAIT_BIT_24_ARGS_R: {
        colorVal = &color_.r;
        state_   = ParseState::STATE_WAIT_BIT_24_ARGS_G;
    } break;
    case ParseState::STATE_WAIT_BIT_24_ARGS_G: {
        colorVal = &color_.g;
        state_   = ParseState::STATE_WAIT_BIT_24_ARGS_B;
    } break;
    case ParseState::STATE_WAIT_BIT_24_ARGS_B: {
        colorVal = &color_.b;
        state_   = bit24Valid ? ParseState::STATE_SUCCESS : ParseState::STATE_CURRENT_COLOR;
    } break;
    default:
        // If the code is not written correctly, it will go here
        state_ = STATE_CODE_ERROR;
        assert( false );
    }

    // bit24Valid is false, no need to convert num and set color
    if ( bit24Valid ) {
        // convert to number, error parameter will break parse, and keep current color
        auto [ ok, value ] = base10ToU8( num );
        if ( !ok ) {
            state_ = STATE_CURRENT_COLOR;
            return ReturnVal::ERROR_AND_BREAK;
        }
        // set color
        *colorVal = value;
        return ReturnVal::SUCCESS;
    }
    else {
        return ReturnVal::ERROR_AND_CONTINUE;
    }
}

} // namespace ANSI
