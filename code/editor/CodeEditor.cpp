#include "CodeEditor.h"

#include "../GUI.H"
#include "../MemoryAllocator.h"
#include "../piplang/Scanner.h"

#include <ctype.h>  // isspace

#include "../singleheaders/stb_textedit.h"

#define FIXED_FONT_WIDTH_HACK 6
#define FIXED_FONT_HEIGHT_HACK 9
#define FIXED_FONT_LINEGAP_HACK 3

STB_TexteditState stbCodeEditorState; // TODO(Kevin): STB_TexteditState per code editor tab / open script

// returns the results of laying out a line of characters starting from character #n (see discussion below)
// STB_TEXTEDIT_LAYOUTROW returns information about the shape of one displayed
// row of characters assuming they start on the i'th character--the width and
// the height and the number of characters consumed. This allows this library
// to traverse the entire layout incrementally. You need to compute word-wrapping
// here.
////////////////////////////////////////////////////////////////////////
//
//     StbTexteditRow
//
// Result of layout query, used by stb_textedit to determine where
// the text in each row is.
// // result of layout query
// typedef struct
// {
//    float x0,x1;             // starting x location, end x location (allows for align=right, etc)
//    float baseline_y_delta;  // position of baseline relative to previous row's baseline
//    float ymin,ymax;         // height of row above and below baseline
//    int num_chars;
// } StbTexteditRow;
void layout_func(StbTexteditRow *row, CodeEditorString *str, int lineStart)
{
    int remaining_chars = str->stringlen - lineStart;
    row->num_chars = remaining_chars;
    for (int i = 0; i < remaining_chars; ++i)
    {
        char c = str->string[lineStart + i];
        if (c == '\n' || c == '\0')
        {
            row->num_chars = i + 1;
            break;
        }
    }
    row->x0 = 0;
    row->x1 = (float)FIXED_FONT_WIDTH_HACK * row->num_chars; // need to account for actual size of characters
    row->baseline_y_delta = FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK;
    row->ymin = -FIXED_FONT_HEIGHT_HACK;
    row->ymax = 0;
}

// delete n characters starting at i
int delete_chars(CodeEditorString *str, int pos, int num)
{
    memmove(&str->string[pos], &str->string[pos+num], str->stringlen - (pos+num));
    str->stringlen -= num;
    return 1; // always succeeds
}

// insert n characters at i (pointed to by STB_TEXTEDIT_CHARTYPE*)
int insert_chars(CodeEditorString *str, int pos, STB_TEXTEDIT_CHARTYPE *newtext, int num)
{
    // ensure enough space is allocated.
    if (str->stringlen + num > str->buf_capacity)
    {
        // bob do something
        //str->string = (char*)realloc(str->string, str->stringlen + num);
    }

    // move chars after cursor pos if any
    if (str->stringlen - pos > 0)
    {
        memmove(&str->string[pos + num], &str->string[pos], str->stringlen - pos);
    }

    // insert new chars
    memcpy(&str->string[pos], newtext, num);
    str->stringlen += num;
    return 1; // always succeeds
}

STB_TEXTEDIT_CHARTYPE key_to_char(STB_TEXTEDIT_KEYTYPE key)
{
    if (key & SDLK_SCANCODE_MASK) return 0;

    char c = (key & 0xFF);

    if (c == 13) c = 10; // Replace CR with LF

    if (' ' <= c && c <= '~' || c == '\n' || c == '\t')
        return (key & STB_TEXTEDIT_K_SHIFT) ? ModifyASCIIBasedOnModifiers(c, true) : c;
    else
        return 0;
}

//#define KEYDOWN_BIT                    0x80000000

#define STB_TEXTEDIT_STRINGLEN(obj)    ((obj)->stringlen)
#define STB_TEXTEDIT_LAYOUTROW         layout_func
#define STB_TEXTEDIT_GETWIDTH(obj,n,i) FIXED_FONT_WIDTH_HACK
#define STB_TEXTEDIT_KEYTOTEXT         key_to_char
#define STB_TEXTEDIT_GETCHAR(obj,i)    ((obj)->string[i])
#define STB_TEXTEDIT_NEWLINE           '\n'
#define STB_TEXTEDIT_IS_SPACE(ch)      isspace(ch)
#define STB_TEXTEDIT_DELETECHARS       delete_chars
#define STB_TEXTEDIT_INSERTCHARS       insert_chars

#define STB_TEXTEDIT_IMPLEMENTATION
#include "../singleheaders/stb_textedit.h"

void GetCursorData(CodeEditorString code, int cursorpos, int *rowcount, int *row, int *col)
{
    if (code.stringlen > 0)
    {
        StbFindState lastlineFind;
        stb_textedit_find_charpos(&lastlineFind, &code, code.stringlen - 1, false);
        *rowcount = int(lastlineFind.y) / (FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK) + 1 + (code.string[code.stringlen - 1] == '\n' ? 1 : 0);
    }
    else
    {
        *rowcount = 1;
    }

    StbFindState find;
    stb_textedit_find_charpos(&find, &code, cursorpos, false);
    int ccol = (int)find.x / FIXED_FONT_WIDTH_HACK;
    int crow = (int)find.y / (FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK);
    if (code.stringlen == cursorpos)
    {
        ccol = (code.stringlen > 0 && code.string[code.stringlen - 1] == '\n') ? 0 : cursorpos - find.prev_first;
        crow = *rowcount - 1;
    }

    *row = crow;
    *col = ccol;
}


// TODO
// int  stb_textedit_cut(STB_TEXTEDIT_STRING *str, STB_TexteditState *state)
// int  stb_textedit_paste(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, STB_TEXTEDIT_CHARTYPE *text, int len)

CodeEditorString GiveMeNewCodeEditorString()
{
    CodeEditorString code;
    code.buf_capacity = 32000;
    code.string = (char*) malloc(code.buf_capacity);
    code.stringlen = 0;
    return code;
}

void SetupCodeEditorString(CodeEditorString *code, const char *initString, u32 len)
{
    memset(code->string, 0, code->buf_capacity);
    code->stringlen = 0;

    if (len > 0)
    {
        memcpy(code->string, initString, len);
        code->stringlen = len;
    }

    stb_textedit_initialize_state(&stbCodeEditorState, 0);
}

const ui_id g_CodeEditorUIID = 0xbc9526f97dff3dec;

const int defaultLineNumbersDisplayWidth = 15;
const int extraLineNumberDigitWidth = FIXED_FONT_WIDTH_HACK;
static int lineNumbersDisplayWidth = defaultLineNumbersDisplayWidth;
int GetTextAnchorOffsetX() { return lineNumbersDisplayWidth + 8; }
const int textAnchorOffsetY = 13;
static int scrollX;
static int scrollY;

void SendMouseScrollToCodeEditor(int x, int y)
{
    scrollX -= x * 8;
    scrollY -= y * 32;
    if (scrollX < 0) scrollX = 0;
    if (scrollY < 0) scrollY = 0;
}

void SendMouseDownToCodeEditor(CodeEditorString *code, int x, int y)
{
    float x_text = (float)(x - GetTextAnchorOffsetX() + scrollX);
    float y_text = (float)y - textAnchorOffsetY + (float)scrollY;
    stb_textedit_click(code, &stbCodeEditorState, x_text, y_text);
}

void SendMouseMoveToCodeEditor(CodeEditorString *code, int x, int y)
{
    float x_text = (float)(x - GetTextAnchorOffsetX() + scrollX);
    float y_text = (float)y - textAnchorOffsetY + (float)scrollY;
    stb_textedit_drag(code, &stbCodeEditorState, x_text, y_text);
}

void SendKeyInputToCodeEditor(CodeEditorString *code, STB_TEXTEDIT_KEYTYPE key)
{
    STB_TexteditState *state = &stbCodeEditorState;

    if ((key & 0xFF) == 0x09) // '\t'
    {
        int rc, r, c;
        GetCursorData(*code, stbCodeEditorState.cursor, &rc, &r, &c);
        stb_textedit_key(code, state, 0x20); // ' '
        if (c % 2 == 0) 
            stb_textedit_key(code, state, 0x20);
    }
    else if (key == (STB_TEXTEDIT_K_CONTROL | STB_TEXTEDIT_K_UP))
    {
        scrollY -= FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK;
        if (scrollY < 0) scrollY = 0;
    }
    else if (key == (STB_TEXTEDIT_K_CONTROL | STB_TEXTEDIT_K_DOWN))
    {
        scrollY += FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK;
    }
    // TODO(Kevin): upon input, ensure cursor is visible by updating scroll values
    else
    {
        stb_textedit_key(code, state, key);
    }
}

static void DrawSelectionHighlightRect(int startCol, int endCol, int row, int zone_x, int zone_y)
{
    int highlight_x_text = startCol * FIXED_FONT_WIDTH_HACK;
    int highlight_y_text = (row - 1) * (FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK) + 1;
    int highlight_x_internal = highlight_x_text + GetTextAnchorOffsetX() + zone_x;
    int highlight_y_internal = highlight_y_text + textAnchorOffsetY + zone_y;
    int highlight_x = highlight_x_internal - scrollX;
    int highlight_y = highlight_y_internal - scrollY;
    int highlight_w = (endCol - startCol) * FIXED_FONT_WIDTH_HACK;
    int highlight_h = (FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK);
    Gui::PrimitivePanel(Gui::UIRect(highlight_x, highlight_y, highlight_w, highlight_h), 3, vec4(0.95f, 0.95f, 0.95f, 0.3f));
}

static void GetRowStartAndEnd(const CodeEditorString code, int row, int *startIndex, int *endIndex)
{
    *startIndex = 0;
    for (int i = 0; i < code.stringlen; ++i)
    {
        char c = code.string[i];
        if (c == '\n' || c == '\0')
        {
            *endIndex = i;
            if (row == 0 || c == '\0')
                return;
            --row;
            *startIndex = i + 1;
        }
    }
}

/*
0xb6b8b9
0xffaed7
*/
//#define CODE_COLOR_OPERATORS        0xffffff
//#define CODE_COLOR_BRACES           0xbcbcbc
//#define CODE_COLOR_STRING_LITERAL   0xadd09d
//#define CODE_COLOR_NUMBER_LITERAL   0x6be6dd
//#define CODE_COLOR_KEYWORD          0xd77bba
//#define CODE_COLOR_COMMENT          0x336530
//#define CODE_COLOR_FN_DECL          0x9dffdf
//#define CODE_COLOR_FN_CALL          0x9dffdf
//#define CODE_COLOR_AFTER_DOT        0xa4c4d9
//#define CODE_COLOR_IDENTIFIER_DEF   0xcad6e5
#define CODE_COLOR_OPERATORS        0xffffff
#define CODE_COLOR_BRACES           0xffffff
#define CODE_COLOR_STRING_LITERAL   0xffffff
#define CODE_COLOR_NUMBER_LITERAL   0xffffff
#define CODE_COLOR_KEYWORD          0xffffff
#define CODE_COLOR_COMMENT          0xffffff
#define CODE_COLOR_FN_DECL          0xffffff
#define CODE_COLOR_FN_CALL          0xffffff
#define CODE_COLOR_AFTER_DOT        0xffffff
#define CODE_COLOR_IDENTIFIER_DEF   0xffffff


static void ColorCode(const char *code)
{
    std::vector<Token> tokens;
    InitScanner(code);

    for (int i = 0; i < 4096 /*TODO(Kevin): too few*/; ++i)
    {
        Token t = PipEditor_ScanToken();

        tokens.push_back(t);

        if (t.type == TokenType::END_OF_FILE)
            break;
    }

    // Note(Kevin): I could do shit like check the next token e.g. if identifier and next token is DOT then identifier is a table
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        bool notLastToken = i + 1 < tokens.size();
        bool notFirstToken = i != 0;
        Token t = tokens[i];
        vec3 color = vec3(0.95f, 0.95f, 0.95f);

        switch (t.type)
        {
            case TokenType::LESS:
            case TokenType::LESS_EQUAL:
            case TokenType::GREATER:
            case TokenType::GREATER_EQUAL:
            case TokenType::EQUAL:
            case TokenType::BANG_EQUAL:
            case TokenType::EQUAL_EQUAL:
            case TokenType::BANG:
            case TokenType::PLUS:
            case TokenType::MINUS:
            case TokenType::ASTERISK:
            case TokenType::FORWARDSLASH:
                color = vec3(RGBHEXTO1(CODE_COLOR_OPERATORS));
                break;
            case TokenType::COMMA:
            case TokenType::DOT:
            case TokenType::COLON:
                color = vec3(RGBHEXTO1(CODE_COLOR_OPERATORS));
                break;

            case TokenType::LSQBRACK:
            case TokenType::RSQBRACK:
                color = vec3(RGBHEXTO1(CODE_COLOR_BRACES));
                break;
            case TokenType::LPAREN:
            case TokenType::RPAREN:
                color = vec3(RGBHEXTO1(CODE_COLOR_BRACES));
                break;
            case TokenType::LBRACE:
            case TokenType::RBRACE:
                color = vec3(RGBHEXTO1(CODE_COLOR_BRACES));
                break;

            case TokenType::STRING_LITERAL:
                color = vec3(RGBHEXTO1(CODE_COLOR_STRING_LITERAL));
                break;
            case TokenType::NUMBER_LITERAL:
            case TokenType::TRUE:
            case TokenType::FALSE:
                color = vec3(RGBHEXTO1(CODE_COLOR_NUMBER_LITERAL));
                break;

            case TokenType::IDENTIFIER:
//                if (notFirstToken && tokens[i - 1].type == TokenType::MUT)
//                {
//                    // new variable declaration
//                    color = vec3(RGBHEXTO1(0x9dffdf));
//                } else
                if (notFirstToken && tokens[i - 1].type == TokenType::FN)
                {
                    // new fn declaration
                    color = vec3(RGBHEXTO1(CODE_COLOR_FN_DECL));
                }
                else if (notFirstToken && tokens[i - 1].type == TokenType::DOT)
                {
                    color = vec3(RGBHEXTO1(CODE_COLOR_AFTER_DOT));
                }
                else if (notLastToken && tokens[i + 1].type == TokenType::LPAREN)
                {
                    // call
                    color = vec3(RGBHEXTO1(CODE_COLOR_FN_CALL));
                }
                else
                {
                    color = vec3(RGBHEXTO1(CODE_COLOR_IDENTIFIER_DEF));
                }
                break;

            case TokenType::AND:
            case TokenType::OR:
            case TokenType::IF:
            case TokenType::ELSE:
            case TokenType::WHILE:
            case TokenType::FOR:
            case TokenType::FN:
            case TokenType::MUT:
            case TokenType::RETURN:
                color = vec3(RGBHEXTO1(CODE_COLOR_KEYWORD));
                break;

            case TokenType::PRINT:
                color = vec3(RGBHEXTO1(CODE_COLOR_KEYWORD));
                break;

            case TokenType::ERROR:
                if (std::string(t.errormsg) == std::string("Unterminated string."))
                {
                    color = vec3(RGBHEXTO1(CODE_COLOR_STRING_LITERAL));
                }
                else if (std::string(t.errormsg) == std::string("comment"))
                {
                    color = vec3(RGBHEXTO1(CODE_COLOR_COMMENT));
                }
                else
                {
                    color = vec3(RGBHEXTO1(0xff0000));
                }
                break;

            case TokenType::END_OF_FILE:
                break;
        }

        size_t startIndex = t.start - code;
        for (size_t i = startIndex; i < startIndex + t.length; ++i)
        {
            Gui::CodeCharIndexToColor[i] = color;
        }
    }
}

void DoCodeEditorGUI(CodeEditorString code)
{
    int x, y, w, h;
    Gui::Window_GetCurrentOffsets(&x, &y);
    Gui::Window_GetWidthHeight(&w, &h);

    Gui::UIRect codeEditorRect = Gui::UIRect(x, y, w - 8, h - 8);

    if (Gui::IsActive(g_CodeEditorUIID))
    {
        if (Gui::keyboardInputASCIIKeycodeThisFrame.Contains(SDLK_ESCAPE))
        {
            Gui::SetActive(null_ui_id);
        }
    }
    else if (Gui::IsHovered(g_CodeEditorUIID))
    {
        if (Gui::MouseWentDown())
        {
            Gui::SetActive(g_CodeEditorUIID);
        }
    }

    if (Gui::MouseInside(codeEditorRect))
    {
        Gui::SetHovered(g_CodeEditorUIID);
    }

    const int codeEditorRectCornerRadius = 0;
    Gui::UIRect rectcopy = codeEditorRect;
    rectcopy.y -= 4;
    //Gui::PrimitivePanel(rectcopy, codeEditorRectCornerRadius, Gui::IsActive(g_CodeEditorUIID) ? vec4(RGB255TO1(40, 44, 52), 1.f) : vec4(RGB255TO1(46, 50, 58), 1.f));
    //Gui::PrimitivePanel(rectcopy, codeEditorRectCornerRadius, Gui::IsActive(g_CodeEditorUIID) ? vec4(RGB255TO1(40, 44, 52), 1.f) : vec4(RGB255TO1(46, 50, 58), 1.f));

    const int textBeginAnchorX = x + GetTextAnchorOffsetX();
    const int textBeginAnchorY = y + textAnchorOffsetY;

    int rowcount, crow, ccol;
    GetCursorData(code, stbCodeEditorState.cursor, &rowcount, &crow, &ccol);

    // Draw cursor
    if (Gui::IsActive(g_CodeEditorUIID))
    {
        if (stbCodeEditorState.insert_mode)
            Gui::PrimitivePanel(Gui::UIRect(textBeginAnchorX - scrollX + ccol * 6, textBeginAnchorY - scrollY - 2 + crow * 12, 5, 2), vec4(1, 1, 1, 1));
        else
            Gui::PrimitivePanel(Gui::UIRect(textBeginAnchorX - scrollX - 1 + ccol * 6, textBeginAnchorY - scrollY - 12 + crow * 12, 1, 13), vec4(1, 1, 1, 1));
    }

    // Draw code
    // I could make each type of text to highlight a different primitive text that is rendered
    // so all keywords are rendered as one set of text batch with one color, all variables rendered as one set with one color, functions, etc.
    if (code.stringlen > 0)
    {
        std::string CodeStdStr = std::string(code.string, code.stringlen);
        // Note(Kevin): map each code character to a color here
        ColorCode(CodeStdStr.c_str());
        Gui::PipCode(textBeginAnchorX - scrollX, textBeginAnchorY - scrollY, 9, CodeStdStr.c_str());
        //Gui::PrimitiveText(textBeginAnchorX - scrollX, textBeginAnchorY - scrollY, 9, Gui::Align::Left, std::string(code.string, code.stringlen).c_str());
    }

    // Draw selection highlight
    int selStart = stbCodeEditorState.select_start;
    int selEnd = stbCodeEditorState.select_end;
    if (selStart != selEnd)
    {
        if (selStart > selEnd) 
        {
            selStart = stbCodeEditorState.select_end;
            selEnd = stbCodeEditorState.select_start;
        }
        int idc, selStartRow, selStartCol, selEndRow, selEndCol;
        GetCursorData(code, selStart, &idc, &selStartRow, &selStartCol);
        GetCursorData(code, selEnd, &idc, &selEndRow, &selEndCol);
        //int highlightedRowCount = selEndRow - selStartRow + 1;
        if (selStartRow == selEndRow)
        {
            DrawSelectionHighlightRect(selStartCol, selEndCol, selStartRow, x, y);
        }
        else
        {
            int rowstart, rowend;
            // draw selStartRow
            GetRowStartAndEnd(code, selStartRow, &rowstart, &rowend);
            DrawSelectionHighlightRect(selStartCol, rowend - rowstart + 1, selStartRow, x, y);
            // draw the rows in between
            for (int i = selStartRow + 1; i < selEndRow; ++i)
            {
                GetRowStartAndEnd(code, i, &rowstart, &rowend);
                DrawSelectionHighlightRect(0, rowend - rowstart + 1, i, x, y);
            }
            // draw selEndRow
            DrawSelectionHighlightRect(0, selEndCol, selEndRow, x, y);
        }
    }

    // Draw line numbers
    int countOfLineNumToDisplay = rowcount; //GM_min((int)rowcount, hackApproximateNumRowsFitInView);
    std::string lineNumbersBuf;
    for (int i = 1; i < countOfLineNumToDisplay + 1; ++i)
    {
        int lineNum = 0/* first line num to display*/ + i;
        lineNumbersBuf += std::to_string(lineNum) + '\n';
    }

    // Set lineNumbersDisplayWidth based on what's drawn
    {
        int hackApproximateNumRowsFitInView = h / (FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK) + 1;
        int hackApproximateNumRowsScrolledDown = scrollY / (FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK);
        int hackLargestDisplayedLineNum = hackApproximateNumRowsScrolledDown + hackApproximateNumRowsFitInView;
        if (hackLargestDisplayedLineNum < 10000)
            lineNumbersDisplayWidth = defaultLineNumbersDisplayWidth + extraLineNumberDigitWidth * 2;
        if (hackLargestDisplayedLineNum < 1000)
            lineNumbersDisplayWidth = defaultLineNumbersDisplayWidth + extraLineNumberDigitWidth;
        if (hackLargestDisplayedLineNum < 100)
            lineNumbersDisplayWidth = defaultLineNumbersDisplayWidth;
    }

    Gui::PrimitivePanel(Gui::UIRect(x - 20, y - 32, lineNumbersDisplayWidth + 20 + 5, h + 64), vec4(0.157f, 0.172f, 0.204f, 1.f));

    vec4 textColorBefore = Gui::style_textColor;
    Gui::style_textColor = vec4(1.f, 1.f, 1.f, 0.38f);
    //Gui::PrimitiveTextMasked(textBeginAnchorX - 8, textBeginAnchorY, 9, Gui::Align::Right, lineNumbersBuf.c_str(), codeEditorRect, codeEditorRectCornerRadius);
    Gui::PrimitiveText(textBeginAnchorX - 5, textBeginAnchorY - scrollY, 9, Gui::Align::Right, lineNumbersBuf.c_str());
    Gui::style_textColor = textColorBefore;
}

