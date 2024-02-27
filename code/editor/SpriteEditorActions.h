#pragma once

#include "SpriteEditor.h"


enum spredit_Action
{
    SPREDIT_OP_PixelsWrite,
};

struct spredit_PixelWriteAction_Data
{
    u32 idx;
    u8 r, g, b, a; //deltas
    u8 signBits;   // 0x01, 0x02, 0x04, 0x08 for +rgba respectively
};


void InitSpriteEditorActionBuffers();
void Undo(SpriteEditorState *state);
void Redo(SpriteEditorState *state);
void ClearRedoBuffer();
void RecordPixelsWrite(spredit_Color *pixelsBefore, spredit_Color *pixelsAfter, i32 w, i32 h);


