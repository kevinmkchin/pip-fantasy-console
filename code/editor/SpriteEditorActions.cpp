#include "SpriteEditorActions.h"
#include "../MesaMath.h"
#include "../ByteBuffer.h"

ByteBuffer undoBuffer;
ByteBuffer redoBuffer;

#define ByteBufferPop(_buffer, T, _val_p) \
do {\
    T* _v = (T*)(_val_p);\
    ByteBuffer *_bb = (_buffer);\
    _bb->position -= sizeof(T);\
    *(_v) = *(T*)(_bb->data + _bb->position);\
} while (0)


static void UndoPixelWriteAction(SpriteColor *pixels, spredit_PixelWriteAction_Data delta)
{
    SpriteColor *pixel = &pixels[delta.idx];
    if (delta.signBits & 0x01) pixel->r -= delta.r;
    else                       pixel->r += delta.r;
    if (delta.signBits & 0x02) pixel->g -= delta.g;
    else                       pixel->g += delta.g;
    if (delta.signBits & 0x04) pixel->b -= delta.b;
    else                       pixel->b += delta.b;
    if (delta.signBits & 0x08) pixel->a -= delta.a;
    else                       pixel->a += delta.a;
}

static void RedoPixelWriteAction(SpriteColor *pixels, spredit_PixelWriteAction_Data delta)
{
    SpriteColor *pixel = &pixels[delta.idx];
    if (delta.signBits & 0x01) pixel->r += delta.r;
    else                       pixel->r -= delta.r;
    if (delta.signBits & 0x02) pixel->g += delta.g;
    else                       pixel->g -= delta.g;
    if (delta.signBits & 0x04) pixel->b += delta.b;
    else                       pixel->b -= delta.b;
    if (delta.signBits & 0x08) pixel->a += delta.a;
    else                       pixel->a -= delta.a;
}

void InitSpriteEditorActionBuffers()
{
    ByteBufferInit(&undoBuffer);
    ByteBufferInit(&redoBuffer);
}

void Undo(SpriteEditorState *state)
{
    if (undoBuffer.position == 0) return;

    spredit_Action op;
    ByteBufferPop(&undoBuffer, spredit_Action, &op);

    switch (op)
    {
        case SPREDIT_OP_PixelsWrite:
        {
            u32 pixelsModifiedCount = 0;
            ByteBufferPop(&undoBuffer, u32, &pixelsModifiedCount);
            for (u32 i = 0; i < pixelsModifiedCount; ++i)
            {
                // read from undo buffer
                spredit_PixelWriteAction_Data delta;
                ByteBufferPop(&undoBuffer, spredit_PixelWriteAction_Data, &delta);
                // reset pixel
                UndoPixelWriteAction(state->frame.pixels, delta);
                // write to redo buffer
                ByteBufferWrite(&redoBuffer, spredit_PixelWriteAction_Data, delta);
            }
            ByteBufferWrite(&redoBuffer, u32, pixelsModifiedCount);
            break;
        }
    }

    ByteBufferWrite(&redoBuffer, spredit_Action, op);
}

void Redo(SpriteEditorState *state)
{
    if (redoBuffer.position == 0) return;

    spredit_Action op;
    ByteBufferPop(&redoBuffer, spredit_Action, &op);

    switch (op)
    {
        case SPREDIT_OP_PixelsWrite:
        {
            u32 pixelsModifiedCount = 0;
            ByteBufferPop(&redoBuffer, u32, &pixelsModifiedCount);
            for (u32 i = 0; i < pixelsModifiedCount; ++i)
            {
                // read from undo buffer
                spredit_PixelWriteAction_Data delta;
                ByteBufferPop(&redoBuffer, spredit_PixelWriteAction_Data, &delta);
                // reset pixel
                RedoPixelWriteAction(state->frame.pixels, delta);
                // write to redo buffer
                ByteBufferWrite(&undoBuffer, spredit_PixelWriteAction_Data, delta);
            }
            ByteBufferWrite(&undoBuffer, u32, pixelsModifiedCount);
            break;
        }
    }

    ByteBufferWrite(&undoBuffer, spredit_Action, op);
}

void ClearRedoBuffer()
{
    redoBuffer.position = 0;
    redoBuffer.size = 0;
}

void RecordPixelsWrite(SpriteColor *pixelsBefore, SpriteColor *pixelsAfter, i32 w, i32 h)
{
#define IsPixelDiff(lhs, rhs) (lhs.r != rhs.r || lhs.g != rhs.g || lhs.b != rhs.b || lhs.a != rhs.a)

    u32 pixelsWrittenCount = 0;

    // for each diff, save new PixelWriteAction_Data
    for (u32 i = 0; i < u32(w * h); ++i)
    {
        if (IsPixelDiff(pixelsBefore[i], pixelsAfter[i]))
        {
            u8 sign = 0x00;
            if (pixelsAfter[i].r >= pixelsBefore[i].r) sign |= 0x01;
            if (pixelsAfter[i].g >= pixelsBefore[i].g) sign |= 0x02;
            if (pixelsAfter[i].b >= pixelsBefore[i].b) sign |= 0x04;
            if (pixelsAfter[i].a >= pixelsBefore[i].a) sign |= 0x08;
            spredit_PixelWriteAction_Data d = {
                i,
                (u8)GM_abs(int(pixelsAfter[i].r) - int(pixelsBefore[i].r)),
                (u8)GM_abs(int(pixelsAfter[i].g) - int(pixelsBefore[i].g)),
                (u8)GM_abs(int(pixelsAfter[i].b) - int(pixelsBefore[i].b)),
                (u8)GM_abs(int(pixelsAfter[i].a) - int(pixelsBefore[i].a)),
                sign
            };
            ByteBufferWrite(&undoBuffer, spredit_PixelWriteAction_Data, d);
            ++pixelsWrittenCount;
        }
    }

    // save count
    ByteBufferWrite(&undoBuffer, u32, pixelsWrittenCount);

    // save op code
    ByteBufferWrite(&undoBuffer, spredit_Action, SPREDIT_OP_PixelsWrite);

#undef IsPixelDiff
}