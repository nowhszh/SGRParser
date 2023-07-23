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
    auto            parseRet = SGRParseContext::ReturnVal::RETURN_SUCCESS_BREAK;

    using ParseResult = SGRParseContext::ParseResult;

    // TODO: optimize
    while ( !seqView.empty()) {
        if ( parseRet == SGRParseContext::ReturnVal::RETURN_ERROR_BREAK ) {
            ret.first = Return::PARSE_ERROR;
            break;
        }

        parseRet = ctx.parse( seqView );

        switch ( ctx.result() ) {
        case ParseResult::RESULT_FRONT_COLOR: {
            ret.first        = Return::PARSE_SUCC;
            ret.second.front = ctx.rgb();
        } break;
        case ParseResult::RESULT_BACK_COLOR: {
            ret.second.back = ctx.rgb();
        } break;
        case ParseResult::RESULT_DEFAULT_COLOR: {
            ret.second = defaultColor_;
        } break;
        case ParseResult::RESULT_CURRENT_COLOR: {
            ret.second = currentColor;
        } break;
            // TODO: check result
        default: {
            return { Return::PARSE_ERROR, currentColor };
        }
        }

        ctx.reset();
    }

    return ret;
}

} // namespace ANSI