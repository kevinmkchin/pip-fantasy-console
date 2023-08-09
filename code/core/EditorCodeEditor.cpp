#include "EditorCodeEditor.h"

#include "MesaIMGUI.h"
#include "MemoryAllocator.h"


u32 _get_cursor(code_editor_state_t *state)
{
    return state->row_offsets_buf[state->cursor_row] + state->cursor_col;
}

u32 GetRowLength(code_editor_state_t *state)
{
    if (state->cursor_row < state->row_offsets_len - 1)
    {
        return state->row_offsets_buf[state->cursor_row + 1] - state->row_offsets_buf[state->cursor_row];
    }
    else
    {
        return state->code_len - state->row_offsets_buf[state->cursor_row];
    }
}

void AllocateMemoryCodeEditorState(code_editor_state_t *state)
{
    if (state->code_buf) free(state->code_buf);
    if (state->row_offsets_buf) free(state->row_offsets_buf);

    state->code_buf_capacity = 32000;
    state->code_buf = (char*) malloc(state->code_buf_capacity);
    state->row_offsets_buf_capacity = 8000;
    state->row_offsets_buf = (u32*) malloc(state->row_offsets_buf_capacity);

    state->code_len = 0;
    state->row_offsets_len = 0;
}

void InitializeCodeEditorState(code_editor_state_t *state, bool reallocMemory, const char *initString, u32 len)
{
    if (reallocMemory) AllocateMemoryCodeEditorState(state);

    memcpy(state->code_buf, initString, len);
    state->code_len = len;
    if (state->code_buf[state->code_len - 1] != '\n')
    {
        state->code_buf[state->code_len] = '\n';
        state->code_len += 1;
    }

    state->row_offsets_len = 0;
    for (u32 i = 0; i < state->code_len; ++i)
    {
        // insert new row offset
        state->row_offsets_buf[state->row_offsets_len] = i;
        state->row_offsets_len += 1;
        // i goes until \n or == state->code_len
        while (i < state->code_len && state->code_buf[i] != '\n') ++i;
    }

    state->cursor_row = 0;
    state->cursor_col = 0;
    state->lasted_edited_cursor_col = 0;
}

void MoveCursor(code_editor_state_t *state, i8 colDirection, i8 rowDirection)
{
    const u32 homeCol = 0;
    const u32 endCol = GetRowLength(state) - 1;

    if (colDirection > 0)
    {
        if (state->cursor_col < endCol)
        {
            state->cursor_col += 1;
        }
        else if (state->cursor_row < state->row_offsets_len - 1)
        {
            state->cursor_row += 1;
            state->cursor_col = 0;
        }
        state->lasted_edited_cursor_col = state->cursor_col;
    }
    else if (colDirection < 0)
    {
        if (state->cursor_col > homeCol)
        {
            state->cursor_col -= 1;
        }
        else if (state->cursor_row > 0)
        {
            state->cursor_row -= 1;
            state->cursor_col = GetRowLength(state) - 1;
        }
        state->lasted_edited_cursor_col = state->cursor_col;
    }

    if (rowDirection > 0)
    {
        if (state->cursor_row < state->row_offsets_len - 1)
        {
            state->cursor_row += 1;
        }
        state->cursor_col = GM_min(state->lasted_edited_cursor_col, GetRowLength(state) - 1);
    }
    else if (rowDirection < 0)
    {
        if (state->cursor_row > 0)
        {
            state->cursor_row -= 1;
        }
        state->cursor_col = GM_min(state->lasted_edited_cursor_col, GetRowLength(state) - 1);
    }
}

void InsertChars(code_editor_state_t *state, char *c, u32 len)
{
    char *const codeBufInsertDest = state->code_buf + _get_cursor(state);
    char *const codeBufMoveDest = codeBufInsertDest + len;
    
    memmove(codeBufMoveDest, codeBufInsertDest, state->code_len - u32(codeBufInsertDest - state->code_buf));
    state->code_len += len;
    memcpy(codeBufInsertDest, c, len);

    state->row_offsets_len = 0;
    for (u32 i = 0; i < state->code_len; ++i)
    {
        // insert new row offset
        state->row_offsets_buf[state->row_offsets_len] = i;
        state->row_offsets_len += 1;
        // i goes until \n or == state->code_len
        while (i < state->code_len && state->code_buf[i] != '\n') ++i;
    }

    u32 offset = u32(codeBufMoveDest - state->code_buf);
    for (u32 row = 0; row < state->row_offsets_len; ++row)
    {
        if (state->row_offsets_buf[row] <= offset && (row+1 == state->row_offsets_len || offset < state->row_offsets_buf[row+1]))
        {
            u32 rowOffset = state->row_offsets_buf[row];
            u32 col = offset - rowOffset;
            state->cursor_row = row;
            state->cursor_col = col;
            state->lasted_edited_cursor_col = col;
        }
    }

    // TODO if about to exceed capacity, then reallocate more
}

void EraseChars(code_editor_state_t *state, u32 beginRow, u32 beginCol, u32 endRow, u32 endCol)
{
    char *const src = state->code_buf + state->row_offsets_buf[endRow] + endCol;
    char *const dst = state->code_buf + state->row_offsets_buf[beginRow] + beginCol;
    ASSERT(src > dst);
    memmove(dst, src, state->code_len - u32(src - state->code_buf));
    state->code_len -= u32(src - dst);

    state->row_offsets_len = 0;
    for (u32 i = 0; i < state->code_len; ++i)
    {
        // insert new row offset
        state->row_offsets_buf[state->row_offsets_len] = i;
        state->row_offsets_len += 1;
        // i goes until \n or == state->code_len
        while (i < state->code_len && state->code_buf[i] != '\n') ++i;
    }

    u32 offset = u32(dst - state->code_buf);
    for (u32 row = 0; row < state->row_offsets_len; ++row)
    {
        if (state->row_offsets_buf[row] <= offset && (row + 1 == state->row_offsets_len || offset < state->row_offsets_buf[row + 1]))
        {
            u32 rowOffset = state->row_offsets_buf[row];
            u32 col = offset - rowOffset;
            state->cursor_row = row;
            state->cursor_col = col;
            state->lasted_edited_cursor_col = col;
        }
    }
}

void EditorCodeEditor(code_editor_state_t *state, u32 width, u32 height, bool enabled)
{
    int x, y;
    MesaGUI::GetXYInZone(&x, &y);
    MesaGUI::UIRect codeEditorRect = MesaGUI::UIRect(x, y, width, height);

    ui_id id = MesaGUI::FreshID();

    if (MesaGUI::IsActive(id))
    {
        if (!enabled)// || MouseWentUp() && !IsHovered(id))
        {
            MesaGUI::SetActive(null_ui_id);
        }
        else 
        {
            for (int i = 0; i < MesaGUI::keyboardInputASCIIKeycodeThisFrame.count; ++i)
            {
                SDL_Keycode keycodeASCII = MesaGUI::keyboardInputASCIIKeycodeThisFrame.At(i);

                if (' ' <= keycodeASCII && keycodeASCII <= '~')
                {
                    InsertChars(state, (char *)&keycodeASCII, 1);
                }
                else if (keycodeASCII == SDLK_BACKSPACE)
                {
                    u32 off, br, bc, er, ec;
                    off = state->row_offsets_buf[state->cursor_row];
                    if (state->cursor_col != 0 || off != 0)
                    {
                        br = state->cursor_row;
                        bc = state->cursor_col - 1;
                        er = state->cursor_row;
                        ec = state->cursor_col;
                        if (state->cursor_col == 0)
                        {
                            br = state->cursor_row - 1;
                            bc = state->row_offsets_buf[er] - state->row_offsets_buf[br] - 1;
                        }
                        EraseChars(state, br, bc, er, ec);
                    }
                }
                else if (keycodeASCII == SDLK_RETURN)
                {
                    char key = '\n';
                    InsertChars(state, &key, 1);
                }
                else if (keycodeASCII == SDLK_TAB)
                {
                    u32 tabAmt = 4 - (state->cursor_col % 4);
                    char key[5] = "    ";
                    InsertChars(state, key, tabAmt);
                    // TODO subtract cursor position along line % 4 (or 2 if i want) from 4 (or 2) and then move by that amount
                }
                else if (keycodeASCII == SDLK_LEFT)
                {
                    MoveCursor(state, -1, 0);
                }
                else if (keycodeASCII == SDLK_RIGHT)
                {
                    MoveCursor(state, 1, 0);
                }
                else if (keycodeASCII == SDLK_DOWN)
                {
                    MoveCursor(state, 0, 1);
                }
                else if (keycodeASCII == SDLK_UP)
                {
                    MoveCursor(state, 0, -1);
                }
                else if (keycodeASCII == SDLK_HOME)
                {
                    // todo
                }
                else if (keycodeASCII == SDLK_END)
                {
                    // todo
                }
            }
        }
    }
    else if (MesaGUI::IsHovered(id))
    {
        if (MesaGUI::MouseWentDown() && enabled)
        {
            MesaGUI::SetActive(id);
        }
    }

    if (MesaGUI::MouseInside(codeEditorRect))
    {
        MesaGUI::SetHovered(id);
    }

    MesaGUI::PrimitivePanel(codeEditorRect, 6, MesaGUI::IsActive(id) ? vec4(RGB255TO1(40, 44, 52), 1.f) : vec4(RGB255TO1(46, 50, 58), 1.f));

    int lineNumbersDisplayWidth = 15;

    int textBeginAnchorX = x + lineNumbersDisplayWidth + 8;
    int textBeginAnchorY = y + 13;

    if (MesaGUI::IsActive(id)) 
    {
        MesaGUI::PrimitivePanel(MesaGUI::UIRect(textBeginAnchorX - 1 + state->cursor_col * 6, textBeginAnchorY-13 + state->cursor_row * 12, 1, 16), vec4(1,1,1,1));
    }
    // I could make each type of text to highlight a different primitive text that is rendered
    // so all keywords are rendered as one set of text batch with one color, all variables rendered as one set with one color, functions, etc. 
    MesaGUI::UIStyle uiss = MesaGUI::GetActiveUIStyleCopy();
    if (state->code_len > 1)
    {
        uiss.textColor = vec4(0.95f, 0.95f, 0.95f, 1.f);
        MesaGUI::PushUIStyle(uiss);
        MesaGUI::PrimitiveText(textBeginAnchorX, textBeginAnchorY, 9, MesaGUI::TextAlignment::Left, std::string(state->code_buf, state->code_len - 1).c_str());
        MesaGUI::PopUIStyle();
    }

    std::string lineNumbersBuf;
    for (int i = 1; i < 44; ++i)
    {
        int lineNum = state->first_visible_row + i;
        lineNumbersBuf += std::to_string(lineNum) + '\n';
    }

    uiss = MesaGUI::GetActiveUIStyleCopy();
    uiss.textColor = vec4(1.f, 1.f, 1.f, 0.38f);
    MesaGUI::PushUIStyle(uiss);
    MesaGUI::PrimitiveText(textBeginAnchorX - 8, textBeginAnchorY, 9, MesaGUI::TextAlignment::Right, lineNumbersBuf.c_str());
    MesaGUI::PopUIStyle();
}

