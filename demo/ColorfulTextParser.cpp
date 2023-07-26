//
// Created by marvin on 23-7-24.
//

#include "ColorfulTextParser.h"

#include <QRegularExpression>
#include <QString>

static QString pattern { "\\x1B\\[([0-9]{0,4}([;:][0-9]{1,3})*)?[mK]" };

using namespace ANSI;

std::vector<CSIFilter::SGRSequence> CSIFilter::filter(std::string& stdText)
{
    QString text(stdText.c_str());

    std::vector<SGRSequence> ansiSeqs = filter(text);

    stdText = text.toStdString();
    return ansiSeqs;
}

std::vector<CSIFilter::SGRSequence> CSIFilter::filter(QString& string)
{
    std::vector<SGRSequence> ansiSeqs;

    QRegularExpression reg(pattern);
    auto               allResult = reg.globalMatch(string);

    size_t removeAnsiCharCnt = 0;
    while (allResult.hasNext()) {
        auto result = allResult.next();
        auto ref    = result.capturedView();

        auto        startPos = result.capturedStart() - removeAnsiCharCnt;
        const auto& bytes    = ref.toUtf8();
        ansiSeqs.emplace_back(startPos, std::string { bytes.constData(), (size_t)bytes.size() });
        removeAnsiCharCnt += result.capturedLength();
    }
    string.remove(reg);
    return ansiSeqs;
}

ColorfulTextParser::ColorfulTextParser(const ANSI::TextAttribute& defaultAttr, const ANSI::TextAttribute& currentAttr)
    : currentTextAttr_(currentAttr)
    , sgrParser_(defaultAttr)
{
}

ColorfulText ColorfulTextParser::parse(QString string, Mode mode)
{
    std::vector<ColorfulText> textList;
    auto                      sgrSeqs = CSIFilter::filter(string);
    const auto&               bytes   = string.toUtf8();
    std::string               stdStr { bytes.constData(), (size_t)bytes.size() };
    if (mode == Mode::ALL_TEXT) {
        allStringToText(textList, sgrSeqs, std::move(stdStr));
    }
    else if (mode == Mode::MARKED_TEXT) {
        markedStringToText(textList, sgrSeqs, std::move(stdStr));
    }

    return textList.front();
}

std::vector<ColorfulText> ColorfulTextParser::parse(const std::vector<QString>& strings, ColorfulTextParser::Mode mode)
{
    std::vector<ColorfulText> textList;
    for (auto string : strings) {
        auto        sgrSeqs = CSIFilter::filter(string);
        const auto& bytes   = string.toUtf8();
        std::string stdStr { bytes.constData(), (size_t)bytes.size() };
        if (mode == Mode::ALL_TEXT) {
            allStringToText(textList, sgrSeqs, std::move(stdStr));
        }
        else if (mode == Mode::MARKED_TEXT) {
            markedStringToText(textList, sgrSeqs, std::move(stdStr));
        }
    }
    return textList;
}

ColorfulText ColorfulTextParser::parse(std::string string, Mode mode)
{
    std::vector<ColorfulText> textList;
    auto                      sgrSeqs = CSIFilter::filter(string);
    if (mode == Mode::ALL_TEXT) {
        allStringToText(textList, sgrSeqs, std::move(string));
    }
    else if (mode == Mode::MARKED_TEXT) {
        markedStringToText(textList, sgrSeqs, std::move(string));
    }
    return textList.front();
}

std::vector<ColorfulText> ColorfulTextParser::parse(const std::vector<std::string>& strings, Mode mode)
{
    std::vector<ColorfulText> textList;
    for (auto string : strings) {
        auto sgrSeqs = CSIFilter::filter(string);
        if (mode == Mode::ALL_TEXT) {
            allStringToText(textList, sgrSeqs, std::move(string));
        }
        else if (mode == Mode::MARKED_TEXT) {
            markedStringToText(textList, sgrSeqs, std::move(string));
        }
    }
    return textList;
}

void ColorfulTextParser::markedStringToText(std::vector<ColorfulText>&                 textList,
                                            const std::vector<CSIFilter::SGRSequence>& sgrSeqs, std::string&& string)
{
    ColorfulText colorfulText { string, {} };
    textList.emplace_back(std::move(colorfulText));
    auto& colors = textList.back().color;

    // empty SGR sequences process
    if (sgrSeqs.empty()) {
        if (currentTextAttr_.state == TextAttribute::State::CUSTOM) {
            TextColorAttr desc { currentTextAttr_.color, 0, string.size() };
            colors.emplace_back(desc);
        }
        return;
    }

    // get first colorful text pos and attribute
    auto firstResult = sgrParser_.parseSGRSequence(currentTextAttr_, sgrSeqs[0].second);
    auto curPos      = sgrSeqs[0].first;
    auto curTextAttr = firstResult.second;

    for (size_t i = 0; i < sgrSeqs.size(); ++i) {
        // if exist next colorful text , parse sequence
        // else set next pos to string end
        size_t                nextPos;
        decltype(curTextAttr) nextTextAttr;
        if (i + 1 < sgrSeqs.size()) {
            auto result  = sgrParser_.parseSGRSequence(curTextAttr, sgrSeqs[i + 1].second);
            nextPos      = sgrSeqs[i + 1].first;
            nextTextAttr = result.second;
        }
        else {
            nextPos      = string.size();
            nextTextAttr = curTextAttr;
        }

        if (curTextAttr.state == TextAttribute::State::CUSTOM) {
            TextColorAttr desc { curTextAttr.color, curPos, nextPos - curPos };
            colors.emplace_back(desc);
        }

        curPos      = nextPos;
        curTextAttr = nextTextAttr;
    }
    currentTextAttr_ = curTextAttr;
}

void ColorfulTextParser::allStringToText(std::vector<ColorfulText>&                 textList,
                                         const std::vector<CSIFilter::SGRSequence>& sgrSeqs, std::string&& string)
{
    ColorfulText colorfulText { string, {} };
    textList.emplace_back(std::move(colorfulText));
    auto& colors = textList.back().color;

    // empty SGR sequences process
    if (sgrSeqs.empty()) {
        TextColorAttr desc { currentTextAttr_.color, 0, string.size() };
        colors.emplace_back(desc);
        return;
    }

    size_t curPos      = 0;
    auto   curTextAttr = currentTextAttr_;

    // first text push back
    auto firstResult  = sgrParser_.parseSGRSequence(curTextAttr, sgrSeqs[0].second);
    auto nextPos      = sgrSeqs[0].first;
    auto nextTextAttr = firstResult.second;
    if (curPos < nextPos) {
        TextColorAttr desc { curTextAttr.color, curPos, nextPos - curPos };
        colors.emplace_back(desc);
    }

    // update context
    curPos      = nextPos;
    curTextAttr = nextTextAttr;

    for (size_t i = 0; i < sgrSeqs.size(); ++i) {
        if (i + 1 < sgrSeqs.size()) {
            auto result  = sgrParser_.parseSGRSequence(curTextAttr, sgrSeqs[i + 1].second);
            nextPos      = sgrSeqs[i + 1].first;
            nextTextAttr = result.second;
        }
        else {
            nextPos      = string.size();
            nextTextAttr = curTextAttr;
        }

        TextColorAttr desc { curTextAttr.color, curPos, nextPos - curPos };
        colors.emplace_back(desc);

        curPos      = nextPos;
        curTextAttr = nextTextAttr;
    }
    currentTextAttr_ = curTextAttr;
}
