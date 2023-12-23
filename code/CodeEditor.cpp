#include "CodeEditor.h"

#include "MesaIMGUI.h"
#include "MemoryAllocator.h"


#include <ctype.h>  // isspace

#include "singleheaders/stb_textedit.h"

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
    row->x1 = FIXED_FONT_WIDTH_HACK * row->num_chars; // need to account for actual size of characters
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
#include "singleheaders/stb_textedit.h"


// TODO
// void stb_textedit_click(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
// void stb_textedit_drag(STB_TEXTEDIT_STRING *str, STB_TexteditState *state, float x, float y)
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

const static ui_id codeEditorUIID = 0xbc9526f97dff3dec;

void SendKeyInputToCodeEditor(CodeEditorString *code, STB_TEXTEDIT_KEYTYPE key)
{
    STB_TexteditState *state = &stbCodeEditorState;

    if (MesaGUI::IsActive(codeEditorUIID))
    {
        stb_textedit_key(code, state, key);
    }
}

void DoCodeEditorGUI(CodeEditorString code)
{
    int x, y, w, h;
    MesaGUI::GetXYInZone(&x, &y);
    MesaGUI::GetWHOfZone(&w, &h);

    MesaGUI::UIRect codeEditorRect = MesaGUI::UIRect(x, y, w - 8, h - 8);


    if (MesaGUI::IsActive(codeEditorUIID))
    {
        if (MesaGUI::Temp_Escape())
        {
            MesaGUI::SetActive(null_ui_id);
        }
    }
    else if (MesaGUI::IsHovered(codeEditorUIID))
    {
        if (MesaGUI::MouseWentDown())
        {
            MesaGUI::SetActive(codeEditorUIID);
        }
    }

    if (MesaGUI::MouseInside(codeEditorRect))
    {
        MesaGUI::SetHovered(codeEditorUIID);
    }

    const int codeEditorRectCornerRadius = 6;
    MesaGUI::PrimitivePanel(codeEditorRect, codeEditorRectCornerRadius, MesaGUI::IsActive(codeEditorUIID) ? vec4(RGB255TO1(40, 44, 52), 1.f) : vec4(RGB255TO1(46, 50, 58), 1.f));

    int lineNumbersDisplayWidth = 15;

    int textBeginAnchorX = x + lineNumbersDisplayWidth + 8;
    int textBeginAnchorY = y + 13;


    int rowcount = 0;
    if (code.stringlen > 0)
    {
        StbFindState lastlineFind;
        stb_textedit_find_charpos(&lastlineFind, &code, code.stringlen - 1, false);
        rowcount = lastlineFind.y / (FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK) + 1 + (code.string[code.stringlen - 1] == '\n' ? 1 : 0);
    }
    else
    {
        rowcount = 1;
    }

    // Draw code
    if (MesaGUI::IsActive(codeEditorUIID))
    {
        StbFindState find;
        stb_textedit_find_charpos(&find, &code, stbCodeEditorState.cursor, false);
        int ccol = find.x / FIXED_FONT_WIDTH_HACK;
        int crow = find.y / (FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK);
        if (code.stringlen == stbCodeEditorState.cursor)
        {
            ccol = (code.stringlen > 0 && code.string[code.stringlen - 1] == '\n') ? 0 : stbCodeEditorState.cursor - find.prev_first;
            crow = rowcount - 1;
        }
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(textBeginAnchorX - 1 + ccol * 6, textBeginAnchorY-13 + crow * 12, 1, 16), vec4(1,1,1,1));
    }
    // I could make each type of text to highlight a different primitive text that is rendered
    // so all keywords are rendered as one set of text batch with one color, all variables rendered as one set with one color, functions, etc. 
    MesaGUI::UIStyle uiss = MesaGUI::GetActiveUIStyleCopy();
    if (code.stringlen > 0)
    {
        uiss.textColor = vec4(0.95f, 0.95f, 0.95f, 1.f);
        MesaGUI::PushUIStyle(uiss);
        MesaGUI::PrimitiveTextMasked(textBeginAnchorX, textBeginAnchorY, 9, MesaGUI::TextAlignment::Left, std::string(code.string, code.stringlen).c_str(), codeEditorRect, codeEditorRectCornerRadius);
        MesaGUI::PopUIStyle();
    }

    // Draw line numbers
    int countOfLineNumToDisplay = rowcount; //GM_min((int)rowcount, h / (FIXED_FONT_HEIGHT_HACK + FIXED_FONT_LINEGAP_HACK) + 1);

    std::string lineNumbersBuf;
    for (int i = 1; i < countOfLineNumToDisplay + 1; ++i)
    {
        int lineNum = 0/* first line num to display*/ + i;
        lineNumbersBuf += std::to_string(lineNum) + '\n';
    }

    uiss = MesaGUI::GetActiveUIStyleCopy();
    uiss.textColor = vec4(1.f, 1.f, 1.f, 0.38f);
    MesaGUI::PushUIStyle(uiss);
    MesaGUI::PrimitiveTextMasked(textBeginAnchorX - 8, textBeginAnchorY, 9, MesaGUI::TextAlignment::Right, lineNumbersBuf.c_str(), codeEditorRect, codeEditorRectCornerRadius);
    MesaGUI::PopUIStyle();
}

