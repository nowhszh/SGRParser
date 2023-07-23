//
// Created by marvin on 23-7-19.
//
#pragma once

#include <string>
#include <utility>

#include "AnsiColor.h"

namespace ANSI {

class SGRParser {
  public:
    using SGRParseReturn = std::pair<Return, TextAttribute>;

  public:
    explicit SGRParser( const TextAttribute& defaultTextAttr );
    ~SGRParser() = default;

    SGRParser( const SGRParser& )            = delete;
    SGRParser( SGRParser&& )                 = delete;
    SGRParser& operator=( const SGRParser& ) = delete;
    SGRParser& operator=( SGRParser&& )      = delete;

    SGRParseReturn parseSGRSequence(
        const TextAttribute& currentTextAttr, const std::string& sequence );

  private:
    TextAttribute defaultTextAttr_;
};

}
