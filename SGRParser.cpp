//
// Created by marvin on 23-7-19.
//

#include "SGRParser.h"
#include "AnsiColor.h"
#include "ColorTable.h"

namespace ANSI {

SGRParser::SGRParser( const Color& defaultColor )
    : defaultColor_ { defaultColor }
{
}

SGRParser::SGRParseReturn SGRParser::parseSGRSequence(
    const Color& currentColor, const std::string& sequence )
{
    // sequence start byte + CSI final byte size error, return current color
    if ( sequence.size() < SequenceStartCnt::HEAD_CNT + 1 ) {
        return { Return::PARSE_ERROR, currentColor };
    }
    // check sequence format
    if ( sequence[ 0 ] != SequenceFirst::EXC || sequence[ 1 ] != SequenceSecond::CSI
        || sequence.back() != CSIFinalBytes::SGR ) {
        return { Return::PARSE_ERROR, currentColor };
    }

    // remove sequence start byte
    std::string_view seqView( sequence );
    seqView.remove_prefix( HEAD_CNT );

    SGRParseReturn  ret { Return::PARSE_SUCC, currentColor };
    SGRParseContext ctx {};
    auto            pos        = seqView.find_first_of( ";:m" );
    bool            need_break = false;

    auto next = [ &pos, &seqView ]() {
        seqView.remove_prefix( pos + 1 );
        pos = seqView.find_first_of( ";:m" );
    };

    // TODO: optimize
    while ( pos != std::string_view::npos ) {
        SGRParseContext::ReturnVal parseRet = SGRParseContext::ReturnVal::SUCCESS;

        switch ( ctx.state() ) {
        case SGRParseContext::STATE_WAIT_FIRST_PARAMETER: {
            std::string_view num { seqView.data(), pos };
            parseRet = ctx.setFirstParameter( num );
        } break;
        case SGRParseContext::STATE_WAIT_VERSION: {
            next();
            std::string_view num { seqView.data(), pos };
            parseRet = ctx.setColorVersion( num );
        } break;
        case SGRParseContext::STATE_WAIT_BIT_8_ARGS: {
            next();
            std::string_view num { seqView.data(), pos };
            parseRet = ctx.setBit8Color( num );
        } break;
        case SGRParseContext::STATE_WAIT_BIT_24_ARGS_R:
        case SGRParseContext::STATE_WAIT_BIT_24_ARGS_G:
        case SGRParseContext::STATE_WAIT_BIT_24_ARGS_B: {
            next();
            std::string_view num { seqView.data(), pos };
            parseRet = ctx.setBit24Color( num );
        } break;
        case SGRParseContext::STATE_SUCCESS: {
            ret.first = Return::PARSE_SUCC;
            if ( ctx.position() == SGRParseContext::FRONT_COLOR ) {
                ret.second.front = ctx.rgb();
            }
            else if ( ctx.position() == SGRParseContext::BACK_COLOR ) {
                ret.second.back = ctx.rgb();
            }
            else {
                assert( false );
            }

            next();
            ctx.reset();
        } break;
        case SGRParseContext::STATE_DEFAULT_COLOR: {
            ret.second = defaultColor_;
            next();
            ctx.reset();
        } break;

        case SGRParseContext::STATE_CURRENT_COLOR: {
            ret.second = currentColor;
            next();
            ctx.reset();
        } break;
        default: {
            return { Return::PARSE_ERROR, currentColor };
        }
        }

        if ( need_break ) {
            break;
        }

        if ( parseRet == SGRParseContext::ReturnVal::ERROR_AND_BREAK ) {
            ret.first  = Return::PARSE_ERROR;
            need_break = true;
            break;
        }
        else if ( parseRet == SGRParseContext::ReturnVal::ERROR_AND_CONTINUE ) {
            next();
        }
    }

    return ret;
}

} // namespace ANSI