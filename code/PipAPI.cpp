#include "PipAPI.h"

#include "GfxRenderer.h"
#include "Timer.h"

HashMap PipAPI_time;
HashMap PipAPI_input;
HashMap PipAPI_gfx;


#define PIPVM_THROW_RUNTIME_ERROR(condition, msg) \
    do {                                          \
        if (condition)                            \
        {                                         \
            PipLangVM_NativeRuntimeError(msg);    \
            return BOOL_VAL(false);               \
        }                                         \
    } while(false);

static TValue GfxRequestSpriteDraw(int argc, TValue *argv)
{
    PIPVM_THROW_RUNTIME_ERROR(argc != 3, "need 3 args");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(argv[0]), "spriteId not a number");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(argv[1]), "x not a number");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(argv[2]), "y not a number");

    i64 spriteId = (i64)AS_NUMBER(argv[0]);
    float fx = (float)AS_NUMBER(argv[1]);
    float fy = (float)AS_NUMBER(argv[2]);

    Gfx::QueueSpriteForRender(spriteId, vec2(fx, fy));

    return BOOL_VAL(true);
}

static TValue GfxClearColor(int argc, TValue *argv)
{
    PIPVM_THROW_RUNTIME_ERROR(argc != 4, "need 4 args");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(argv[0]), "r not a number");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(argv[1]), "g not a number");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(argv[2]), "b not a number");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(argv[3]), "a not a number");

    float fr = (float)AS_NUMBER(argv[0]);
    float fg = (float)AS_NUMBER(argv[1]);
    float fb = (float)AS_NUMBER(argv[2]);
    float fa = (float)AS_NUMBER(argv[3]);

    Gfx::SetGameLayerClearColor(vec4(fr, fg, fb, fa));
    return BOOL_VAL(true);
}

void InitializePipAPI()
{
    if (vm.globals.entries == NULL)
    {
        printf("pip API: attempting to initialize pip API without initializing pip VM!");
        return;
    }

    PipAPI_time = HashMap();
    AllocateHashMap(&PipAPI_time);
    ++PipAPI_time.base.refCount;
    HashMapSet(&vm.globals, CopyString("time", 4, true), RCOBJ_VAL((RCObject*)&PipAPI_time), NULL);

    HashMapSet(&PipAPI_time, CopyString("dt", 2, true), NUMBER_VAL(Time.deltaTime), NULL);

    PipAPI_gfx = HashMap();
    AllocateHashMap(&PipAPI_gfx);
    ++PipAPI_gfx.base.refCount;
    HashMapSet(&vm.globals, CopyString("gfx", 3, true), RCOBJ_VAL((RCObject*)&PipAPI_gfx), NULL);

    PipLangVM_DefineNativeFn(&PipAPI_gfx, "clear", GfxClearColor);
    PipLangVM_DefineNativeFn(&PipAPI_gfx, "sprite", GfxRequestSpriteDraw);
}

void UpdatePipAPI()
{
    HashMapSet(&PipAPI_time, CopyString("dt", 2, true), NUMBER_VAL(Time.deltaTime), NULL);
}

void TeardownPipAPI()
{

}
