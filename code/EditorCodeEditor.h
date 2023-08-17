#pragma once

#include "MesaCommon.h"
#include "MesaMath.h"

struct code_editor_state_t
{
    /*
    row_offsets_buf is array with offsets from base (code_buf) to rows
        offsets are inclusive
    */

    // data
    char* code_buf = NULL;
    u32 code_len = 0;
    u32 code_buf_capacity = 0;
    u32* row_offsets_buf = NULL;
    u32 row_offsets_len = 0;
    u32 row_offsets_buf_capacity = 0;

    // malloc and realloc string_buf
    // malloc and realloc row_offsets_buf

    // editing info
    u32 cursor_row = 0;
    u32 cursor_col = 0;
    /*
    u32 selection_begin_row;
    u32 selection_begin_col;
    u32 selection_end_row;
    u32 selection_end_col;
    // cursor (no selection) mode would be if begin_row == end_row && begin_col == end_col
    */
    u32 lasted_edited_cursor_col = 0;

    // display info
    u32 first_visible_row = 0;
};

void AllocateMemoryCodeEditorState(code_editor_state_t* state);
void InitializeCodeEditorState(code_editor_state_t *state, bool reallocMemory, const char* initString, u32 len);
void EditorCodeEditor(code_editor_state_t *state, u32 width, u32 height, bool enabled);
