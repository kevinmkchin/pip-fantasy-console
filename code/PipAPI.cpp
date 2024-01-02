#include "PipAPI.h"

#include "GfxRenderer.h"
#include "Timer.h"
#include "Input.h"

HashMap PipAPI_time;
HashMap PipAPI_input;
HashMap PipAPI_gfx;
HashMap PipAPI_math;


#define PIPVM_THROW_RUNTIME_ERROR(condition, msg) \
    do {                                          \
        if (condition)                            \
        {                                         \
            PipLangVM_NativeRuntimeError(msg);    \
            return BOOL_VAL(false);               \
        }                                         \
    } while(false);

static TValue GfxDrawRect(int argc, TValue *argv)
{
    PIPVM_THROW_RUNTIME_ERROR(argc != 2, "draw rect expects 2 args");
    PIPVM_THROW_RUNTIME_ERROR(!RCOBJ_IS_MAP(argv[0]), "expect rect as first arg");
    PIPVM_THROW_RUNTIME_ERROR(!RCOBJ_IS_MAP(argv[1]), "expect color as second arg");

    HashMap *rect = RCOBJ_AS_MAP(argv[0]);
    HashMap *color = RCOBJ_AS_MAP(argv[1]);

    TValue v;
    PIPVM_THROW_RUNTIME_ERROR(!HashMapGet(rect, CopyString("x", 1, true), &v), "rect does not have x entry");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "rect.x is not a number");
    float x = (float)AS_NUMBER(v);
    PIPVM_THROW_RUNTIME_ERROR(!HashMapGet(rect, CopyString("y", 1, true), &v), "rect does not have y entry");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "rect.y is not a number");
    float y = (float)AS_NUMBER(v);
    PIPVM_THROW_RUNTIME_ERROR(!HashMapGet(rect, CopyString("w", 1, true), &v), "rect does not have w entry");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "rect.w is not a number");
    float w = (float)AS_NUMBER(v);
    PIPVM_THROW_RUNTIME_ERROR(!HashMapGet(rect, CopyString("h", 1, true), &v), "rect does not have h entry");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "rect.h is not a number");
    float h = (float)AS_NUMBER(v);

    PIPVM_THROW_RUNTIME_ERROR(!HashMapGet(color, CopyString("r", 1, true), &v), "color does not have r entry");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "color.r is not a number");
    float r = (float)AS_NUMBER(v);
    PIPVM_THROW_RUNTIME_ERROR(!HashMapGet(color, CopyString("g", 1, true), &v), "color does not have g entry");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "color.g is not a number");
    float g = (float)AS_NUMBER(v);
    PIPVM_THROW_RUNTIME_ERROR(!HashMapGet(color, CopyString("b", 1, true), &v), "color does not have b entry");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "color.b is not a number");
    float b = (float)AS_NUMBER(v);
    float a = 255.f;
    if (HashMapGet(color, CopyString("a", 1, true), &v))
    {
        PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "color.a is not a number");
        a = (float)AS_NUMBER(v);
    }

    Gfx::Primitive_DrawRect(x, y, w, h, vec4(r,g,b,a)/255.f);

    return BOOL_VAL(true);
}

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
    PIPVM_THROW_RUNTIME_ERROR(argc != 1, "expect 1 color arg");
    PIPVM_THROW_RUNTIME_ERROR(!RCOBJ_IS_MAP(argv[0]), "expect color as arg");
    HashMap *color = RCOBJ_AS_MAP(argv[0]);
    TValue v;
    PIPVM_THROW_RUNTIME_ERROR(!HashMapGet(color, CopyString("r", 1, true), &v), "color does not have r entry");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "color.r is not a number");
    float r = (float)AS_NUMBER(v);
    PIPVM_THROW_RUNTIME_ERROR(!HashMapGet(color, CopyString("g", 1, true), &v), "color does not have g entry");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "color.g is not a number");
    float g = (float)AS_NUMBER(v);
    PIPVM_THROW_RUNTIME_ERROR(!HashMapGet(color, CopyString("b", 1, true), &v), "color does not have b entry");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "color.b is not a number");
    float b = (float)AS_NUMBER(v);
    float a = 255.f;
    if (HashMapGet(color, CopyString("a", 1, true), &v))
    {
        PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(v), "color.a is not a number");
        a = (float)AS_NUMBER(v);
    }

    Gfx::SetGameLayerClearColor(vec4(r, g, b, a));
    return BOOL_VAL(true);
}

static TValue MathCos(int argc, TValue *argv)
{
    PIPVM_THROW_RUNTIME_ERROR(argc != 1, "expect 1 number");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(argv[0]), "math.cos expect first arg to be number");
    return NUMBER_VAL(cos(AS_NUMBER(argv[0])));
}

static TValue MathSin(int argc, TValue *argv)
{
    PIPVM_THROW_RUNTIME_ERROR(argc != 1, "expect 1 number");
    PIPVM_THROW_RUNTIME_ERROR(!IS_NUMBER(argv[0]), "math.sin expect first arg to be number");
    return NUMBER_VAL(sin(AS_NUMBER(argv[0])));
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

    PipAPI_input = HashMap();
    AllocateHashMap(&PipAPI_input);
    ++PipAPI_input.base.refCount;
    HashMapSet(&vm.globals, CopyString("ctrl", 4, true), RCOBJ_VAL((RCObject*)&PipAPI_input), NULL);

    HashMapSet(&PipAPI_input, CopyString("left", 4, true), BOOL_VAL(false), NULL);
    HashMapSet(&PipAPI_input, CopyString("right", 5, true), BOOL_VAL(false), NULL);
    HashMapSet(&PipAPI_input, CopyString("up", 2, true), BOOL_VAL(false), NULL);
    HashMapSet(&PipAPI_input, CopyString("down", 4, true), BOOL_VAL(false), NULL);
    HashMapSet(&PipAPI_input, CopyString("w", 1, true), BOOL_VAL(false), NULL);
    HashMapSet(&PipAPI_input, CopyString("s", 1, true), BOOL_VAL(false), NULL);

    PipAPI_gfx = HashMap();
    AllocateHashMap(&PipAPI_gfx);
    ++PipAPI_gfx.base.refCount;
    HashMapSet(&vm.globals, CopyString("gfx", 3, true), RCOBJ_VAL((RCObject*)&PipAPI_gfx), NULL);

    PipLangVM_DefineNativeFn(&PipAPI_gfx, "clear", GfxClearColor);
    PipLangVM_DefineNativeFn(&PipAPI_gfx, "sprite", GfxRequestSpriteDraw);
    PipLangVM_DefineNativeFn(&PipAPI_gfx, "drawrect", GfxDrawRect);

    PipAPI_math = HashMap();
    AllocateHashMap(&PipAPI_math);
    ++PipAPI_math.base.refCount;
    HashMapSet(&vm.globals, CopyString("math", 4, true), RCOBJ_VAL((RCObject*)&PipAPI_math), NULL);

    PipLangVM_DefineNativeFn(&PipAPI_math, "cos", MathCos);
    PipLangVM_DefineNativeFn(&PipAPI_math, "sin", MathSin);
}

void UpdatePipAPI()
{
    HashMapSet(&PipAPI_time, CopyString("dt", 2, true), NUMBER_VAL(Time.deltaTime), NULL);

    HashMapSet(&PipAPI_input, CopyString("left", 4, true), BOOL_VAL(Input.KeyPressed(SDL_SCANCODE_LEFT)), NULL);
    HashMapSet(&PipAPI_input, CopyString("right", 5, true), BOOL_VAL(Input.KeyPressed(SDL_SCANCODE_RIGHT)), NULL);
    HashMapSet(&PipAPI_input, CopyString("up", 2, true), BOOL_VAL(Input.KeyPressed(SDL_SCANCODE_UP)), NULL);
    HashMapSet(&PipAPI_input, CopyString("down", 4, true), BOOL_VAL(Input.KeyPressed(SDL_SCANCODE_DOWN)), NULL);
    HashMapSet(&PipAPI_input, CopyString("w", 1, true), BOOL_VAL(Input.KeyPressed(SDL_SCANCODE_W)), NULL);
    HashMapSet(&PipAPI_input, CopyString("s", 1, true), BOOL_VAL(Input.KeyPressed(SDL_SCANCODE_S)), NULL);
}

void TeardownPipAPI()
{

}
