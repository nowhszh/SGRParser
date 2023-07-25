//
// Created by marvin on 23-7-24.
//

#pragma once

#include <vector>

#include <QString>

#include "SGRParser.h"

struct TextColorAttr {
    ANSI::Color color;
    size_t      start;
    size_t      len;
};

struct ColorfulText {
    std::string                text;
    std::vector<TextColorAttr> color;
};

class CSIFilter {
public:
    CSIFilter()  = default;
    ~CSIFilter() = default;

    // ANSI: start, data
    using SGRSequence = std::pair<size_t, std::string>;

    static std::vector<SGRSequence> filter(QString& stdText);
    static std::vector<SGRSequence> filter(std::string& stdText);
};

class ColorfulTextParser {
public:
    enum class Mode {
        MARKED_TEXT,
        ALL_TEXT,
    };

public:
    explicit ColorfulTextParser(const ANSI::TextAttribute& defaultAttr, const ANSI::TextAttribute& currentAttr);

    std::vector<ColorfulText> parse(QString strings, Mode mode = Mode::ALL_TEXT);

    std::vector<ColorfulText> parse(std::string strings, Mode mode = Mode::ALL_TEXT);

    std::vector<ColorfulText> parse(const std::vector<std::string>& strings, Mode mode = Mode::ALL_TEXT);

private:
    void markedStringToText(std::vector<ColorfulText>& textList, const std::vector<CSIFilter::SGRSequence>& sgrSeqs,
                            std::string&& string);
    void allStringToText(std::vector<ColorfulText>& textList, const std::vector<CSIFilter::SGRSequence>& sgrSeqs,
                         std::string&& string);

private:
    ANSI::TextAttribute currentTextAttr_;
    ANSI::SGRParser     sgrParser_;
};
