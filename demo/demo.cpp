#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <qlogging.h>

#include <cinttypes>

#include "ColorfulTextParser.h"
#include "SGRParser.h"

using namespace ANSI;

// test ansi sequence
std::vector<std::string> vec {
    "\033[31mhe\033[0mll\033[32mo\033[m",
    "hello",
    // 3/4-bit colors
    "\033[31mhello\033[0m",
    "\033[0;32mhello\033[0m",
    "\033[1;33mhello\033[0m",
    "\033[31;42mhello\033[0m",
    "\033[1;32;43mhello\033[0m",
    "\033[1;34;40mhello",
    "hell\033[31;43mo\033[0m",
    // 8-bit colors
    "\033[38;5;160mhello\033[m",
    "\033[0;38;5;160;48;5;19mhello\033[m",
    "\033[1;38;5;36mhello\033[m",
    // 24-bit colors
    "\033[38;2;215;0;0mhello\033[m",
    "\033[0;38;2;215;0;0;48;2;95;135;175mhello\033[m",
    "\033[1;38;2;95;135;175;48;2;215;0;0mhello\033[m",
};

QDebug& operator<<(QDebug& o, const ANSI::RGB& rgb)
{
    // color value len*3 + ,*2 + () + \0
    constexpr auto len = 3 * 3 + 2 + 2 + 2;
    char           rgbString[len] {};
    snprintf(rgbString, len - 1, "(%" PRIu8 ",%" PRIu8 ",%" PRIu8 ")", rgb.r, rgb.g, rgb.b);
    o << rgbString;
    return o;
}

class MyWindow : public QMainWindow {
private:
    Color defaultColor_;

public:
    MyWindow()
        : QMainWindow(nullptr)
        , defaultColor_ {
            { 0, 0, 0 },
            { 255, 255, 255 },
        }
    {
    }

    void paintEvent(QPaintEvent* event) override
    {
        // set font and init painter
        auto font = this->font();
        font.setPixelSize(50);
        this->setFont(font);

        QPainter painter(this);

        // set initial start point
        constexpr int initX = 50;
        constexpr int initY = 50;

        constexpr int margin       = 3;
        int           maxTextWidth = 0;
        int           fontHeight   = painter.fontMetrics().height();

        // Text: description string and the color range
        // parse ansi color from string, then storage to Text
        ColorfulTextParser parser({ TextAttribute::State::DEFAULT, defaultColor_ },
                                  { TextAttribute::State::DEFAULT, defaultColor_ });
        //        std::vector<ColorfulText> textList;
        //        for (auto& str : vec) {
        //            auto ret = parser.parse({ str.c_str(), str.size() }, ColorfulTextParser::Mode::MARKED_TEXT);
        //            textList.emplace_back(ret);
        //        }

        std::vector<ColorfulText> textList = parser.parse(vec);

        int y = initY;
        for (auto& text : textList) {
            auto& string = text.text;
            int   x      = initX;

            int testWidth = painter.fontMetrics().horizontalAdvance(QString(string.c_str()));
            maxTextWidth  = testWidth > maxTextWidth ? testWidth : maxTextWidth;
            for (auto& textColor : text.color) {
                auto substr = string.substr(textColor.start, textColor.len);

                // record max string width

                auto& front = textColor.color.front;
                auto& back  = textColor.color.back;
                qDebug() << "front:" << front;
                qDebug() << "back:" << back;

                int width = painter.fontMetrics().horizontalAdvance(QString(substr.c_str()));

                // draw background
                auto backColor = QColor(back.r, back.g, back.b);
                painter.fillRect(x, y, width, fontHeight + margin, backColor);

                // draw text
                auto frontColor = QColor(front.r, front.g, front.b);
                painter.setPen(frontColor);
                painter.drawText(x, y + fontHeight, substr.c_str());

                // move to next position
                x += width;
            }
            // move to next line
            y += (fontHeight + margin);
        }

        // set window height and width
        auto windowHeight = fontHeight * (int)vec.size() + 2 * initY;
        this->setFixedHeight(windowHeight);
        auto windowWidth = maxTextWidth + initY * 2;
        this->setFixedWidth(windowWidth);
    }
};

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    MyWindow mw;

    mw.show();

    return QApplication::exec();
}
