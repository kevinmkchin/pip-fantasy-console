#pragma once

#include "SDL.h"

#include "../MesaCommon.h"
#include "../MesaMath.h"
#include "../GUI.H"

#define STB_TEXTEDIT_KEYTYPE    int
#define STB_TEXTEDIT_CHARTYPE   char
#define STB_TEXTEDIT_STRING     CodeEditorString

#define STB_TEXTEDIT_K_SHIFT           0x10000000
#define STB_TEXTEDIT_K_CONTROL         0x20000000
#define STB_TEXTEDIT_K_LEFT            SDLK_LEFT
#define STB_TEXTEDIT_K_RIGHT           SDLK_RIGHT
#define STB_TEXTEDIT_K_UP              SDLK_UP
#define STB_TEXTEDIT_K_DOWN            SDLK_DOWN
#define STB_TEXTEDIT_K_LINESTART       SDLK_HOME
#define STB_TEXTEDIT_K_LINEEND         SDLK_END
#define STB_TEXTEDIT_K_TEXTSTART       (STB_TEXTEDIT_K_LINESTART | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_TEXTEND         (STB_TEXTEDIT_K_LINEEND   | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_DELETE          SDLK_DELETE
#define STB_TEXTEDIT_K_BACKSPACE       SDLK_BACKSPACE
#define STB_TEXTEDIT_K_UNDO            (STB_TEXTEDIT_K_CONTROL | SDLK_z)
#define STB_TEXTEDIT_K_REDO            (STB_TEXTEDIT_K_CONTROL | STB_TEXTEDIT_K_SHIFT | SDLK_z)
#define STB_TEXTEDIT_K_INSERT          SDLK_INSERT
#define STB_TEXTEDIT_K_WORDLEFT        (STB_TEXTEDIT_K_LEFT  | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_WORDRIGHT       (STB_TEXTEDIT_K_RIGHT | STB_TEXTEDIT_K_CONTROL)
#define STB_TEXTEDIT_K_PGUP            SDLK_PAGEUP
#define STB_TEXTEDIT_K_PGDOWN          SDLK_PAGEDOWN

struct CodeEditorString
{
    char *string;
    int stringlen;
    int buf_capacity = 0;
};

CodeEditorString GiveMeNewCodeEditorString();
void SetupCodeEditorString(CodeEditorString *code, const char *initString, u32 len);
void SendMouseDownToCodeEditor(CodeEditorString *code, int x, int y);
void SendMouseMoveToCodeEditor(CodeEditorString *code, int x, int y);
void SendMouseScrollToCodeEditor(int x, int y);
void SendKeyInputToCodeEditor(CodeEditorString *code, STB_TEXTEDIT_KEYTYPE key);
void DoCodeEditorGUI(CodeEditorString code);

extern const ui_id g_CodeEditorUIID;
