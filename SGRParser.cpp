//
// Created by marvin on 23-7-19.
//

#include "SGRParser.h"
#include "AnsiColor.h"
#include "ColorTable.h"

namespace ANSI {

SGRParser::SGRParser( const TextAttribute& defaultTextAttr )
    : defaultTextAttr_ { defaultTextAttr }
{
}

SGRParser::SGRParseReturn SGRParser::parseSGRSequence(
    const TextAttribute& currentTextAttr, const std::string& sequence )
{
    // sequence start byte + CSI final byte size error, return current color
    if ( sequence.size() < SequenceStartCnt::HEAD_CNT + 1 ) {
        return { Return::PARSE_ERROR, currentTextAttr };
    }
    // check sequence format
    if ( sequence[ 0 ] != SequenceFirst::EXC || sequence[ 1 ] != SequenceSecond::CSI
        || sequence.back() != CSIFinalBytes::SGR ) {
        return { Return::PARSE_ERROR, currentTextAttr };
    }
    // remove sequence start byte
    std::string_view seqView( sequence );
    seqView.remove_prefix( HEAD_CNT );

    SGRParseContext            ctx {};
    SGRParseContext::ReturnVal ctxRet;
    SGRParseReturn             ret { Return::PARSE_SUCC, currentTextAttr };

    using ParseResult = SGRParseContext::ParseResult;
    while ( !seqView.empty() ) {
        // continuous parsing and logging of results at each step
        ctxRet = ctx.parse( seqView );
        switch ( ctx.result() ) {
        case ParseResult::RESULT_FRONT_COLOR: {
            ret.second.color.front = ctx.color();
        } break;
        case ParseResult::RESULT_BACK_COLOR: {
            ret.second.color.back = ctx.color();
        } break;
        case ParseResult::RESULT_DEFAULT_FRONT_COLOR: {
            ret.second.color.front = defaultTextAttr_.color.front;
        } break;
        case ParseResult::RESULT_DEFAULT_BACK_COLOR: {
            ret.second.color.back = defaultTextAttr_.color.back;
        } break;
        case ParseResult::RESULT_DEFAULT_TEXT_ATTR: {
            ret.second = defaultTextAttr_;
        } break;
        case ParseResult::RESULT_CURRENT_TEXT_ATTR:
        case ParseResult::RESULT_UNSUPPORTED_ATTR: {
            ret.second = currentTextAttr;
        } break;
        }

        ctx.reset();

        // RETURN_ERROR_BREAK aborts parsing and invalidates parsed results
        if ( ctxRet == SGRParseContext::ReturnVal::RETURN_ERROR_BREAK ) {
            ret = { Return::PARSE_ERROR, currentTextAttr };
            break;
        }
    }

    return ret;
}

} // namespace ANSI