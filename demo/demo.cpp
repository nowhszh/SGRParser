#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRegularExpression>
#include <qlogging.h>

#include <cinttypes>
#include <regex>
#include <string>

#include "SGRParser.h"

using namespace ANSI;

static QString           pattern { "\\x1B\\[([0-9]{0,4}((;|:)[0-9]{1,3})*)?[mK]" };
// test ansi sequence
std::vector<std::string> vec {
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

QDebug& operator<<( QDebug& o, const ANSI::RGB& rgb )
{
    // color value len*3 + ,*2 + () + \0
    constexpr auto len = 3 * 3 + 2 + 2 + 2;
    char           rgbString[ len ] {};
    snprintf( rgbString, len - 1, "(%" PRIu8 ",%" PRIu8 ",%" PRIu8 ")", rgb.r, rgb.g, rgb.b );
    o << rgbString;
    return o;
}

class AnsiColorFilter {
  public:
    AnsiColorFilter()  = default;
    ~AnsiColorFilter() = default;

    // ANSI: start, data
    using SGRSequence = std::pair<size_t, std::string>;

    static std::vector<SGRSequence> filter( std::string& stdText )
    {
        std::vector<SGRSequence> ansiSeqs;
        QString                  text( stdText.c_str() );

        QRegularExpression reg( pattern );
        auto               allResult = reg.globalMatch( text );

        size_t removeAnsiCharCnt = 0;
        while ( allResult.hasNext() ) {
            auto result = allResult.next();
            auto ref    = result.capturedView();

            auto        startPos = result.capturedStart() - removeAnsiCharCnt;
            const auto& bytes    = ref.toUtf8();
            ansiSeqs.emplace_back(
                startPos, std::string { bytes.constData(), (size_t)bytes.size() } );
            removeAnsiCharCnt += result.capturedLength();
        }
        text.remove( reg );
        stdText = std::move( text.toStdString() );
        return std::move( ansiSeqs );
    }
};

struct TextColorAttr {
    Color  color;
    size_t start;
    size_t len;
};

struct ColorfulText {
    std::string                text;
    std::vector<TextColorAttr> color;
};

class ColorfulTextParser {
  private:
    TextAttribute currentTextAttr_;
    SGRParser     sgrParser_;

  public:
    explicit ColorfulTextParser(
        const TextAttribute& defaultAttr, const TextAttribute& currentAttr )
        : currentTextAttr_( currentAttr )
        , sgrParser_( defaultAttr )
    {
    }

    [[nodiscard]] inline Color currentColor() const
    {
        return currentTextAttr_.color;
    }

    std::vector<ColorfulText> parseColor( std::vector<std::string> strings )
    {
        std::vector<ColorfulText> textList;
        for ( auto& string : strings ) {
            auto ansiSeqs = AnsiColorFilter::filter( string );
            stringToText( textList, ansiSeqs, string );
        }
        return std::move( textList );
    }

  private:
    // TODO: to optimize
    void stringToText( std::vector<ColorfulText>&        textList,
        const std::vector<AnsiColorFilter::SGRSequence>& ansiSeqs, const std::string& string )
    {
        if ( ansiSeqs.empty() ) {
            TextColorAttr desc { currentTextAttr_.color, 0, string.size() };
            textList.emplace_back( ColorfulText { string, { desc } } );
            return;
        }

        size_t curPos = 0;
        size_t i      = 0;

        ColorfulText textInfo { string };

        auto* curAnsi         = &ansiSeqs[ i ];
        auto [ _, nextColor ] = sgrParser_.parseSGRSequence( currentTextAttr_, curAnsi->second );
        auto nextPos          = curAnsi->first;
        if ( curPos < nextPos ) {
            TextColorAttr desc { currentTextAttr_.color, 0, nextPos };
            textInfo.color.emplace_back( desc );
        }
        currentTextAttr_ = nextColor;
        curPos           = nextPos;
        (void)_;

        while ( i < ansiSeqs.size() ) {
            if ( i + 1 < ansiSeqs.size() ) {
                curAnsi     = &ansiSeqs[ i + 1 ];
                auto result = sgrParser_.parseSGRSequence( currentTextAttr_, curAnsi->second );
                nextColor   = result.second;
                nextPos     = curAnsi->first;
            }
            else {
                nextPos   = string.size();
                nextColor = currentTextAttr_;
            }

            TextColorAttr desc { currentTextAttr_.color, curPos, nextPos - curPos };
            textInfo.color.emplace_back( desc );
            currentTextAttr_ = nextColor;
            curPos           = nextPos;
            ++i;
        }

        textList.emplace_back( textInfo );
    }
};

class MyWindow : public QMainWindow {
  private:
    Color defaultColor_;

  public:
    MyWindow()
        : QMainWindow( nullptr )
        , defaultColor_ {
            { 0, 0, 0 },
            { 255, 255, 255 },
        }
    {
    }

    void paintEvent( QPaintEvent* event ) override
    {
        // set font and init painter
        auto font = this->font();
        font.setPixelSize( 50 );
        this->setFont( font );

        QPainter painter( this );

        // set initial start point
        constexpr int initX = 50;
        constexpr int initY = 50;

        constexpr int margin       = 3;
        int           maxTextWidth = 0;
        int           fontHeight   = painter.fontMetrics().height();

        // Text: description string and the color range
        // parse ansi color from string, then storage to Text
        ColorfulTextParser        parser( { defaultColor_ }, { defaultColor_ } );
        std::vector<ColorfulText> textList = parser.parseColor( vec );

        int y = initY;
        for ( auto& text : textList ) {
            auto& string = text.text;
            int   x      = initX;

            for ( auto& textColor : text.color ) {
                auto substr = string.substr( textColor.start, textColor.len );

                // record max string width
                int width    = painter.fontMetrics().horizontalAdvance( QString( substr.c_str() ) );
                maxTextWidth = width > maxTextWidth ? width : maxTextWidth;

                auto& front = textColor.color.front;
                auto& back  = textColor.color.back;
                qDebug() << "front:" << front;
                qDebug() << "back:" << back;

                // draw background
                auto backColor = QColor( back.r, back.g, back.b );
                painter.fillRect( x, y, width, fontHeight + margin, backColor );

                // draw text
                auto frontColor = QColor( front.r, front.g, front.b );
                painter.setPen( frontColor );
                painter.drawText( x, y + fontHeight, substr.c_str() );

                // move to next position
                x += width;
            }
            // move to next line
            y += ( fontHeight + margin );
        }

        // set window height and width
        auto windowHeight = fontHeight * (int)vec.size() + 2 * initY;
        this->setFixedHeight( windowHeight );
        auto windowWidth = maxTextWidth + initY * 2;
        this->setFixedWidth( windowWidth );
    }
};

int main( int argc, char* argv[] )
{
    QApplication a( argc, argv );

    MyWindow mw;

    mw.show();

    return QApplication::exec();
}
