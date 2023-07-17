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
    using SGRParseReturn = std::pair<Return, Color>;

  public:
    explicit SGRParser( const Color& defaultColor );
    ~SGRParser() = default;

    SGRParseReturn parseSGRSequence( const Color& currentColor, const std::string& sequence );

  private:
    Color defaultColor_;
};

}
