#include "PipAPI.h"

#include "MesaScript.h"
#include "GfxRenderer.h"


TValue Temp_RaiseTo(TValue base, TValue exponent)
{
    ASSERT(base.type == TValue::ValueType::Integer);
    ASSERT(exponent.type == TValue::ValueType::Integer);

    TValue result;
    result.type = TValue::ValueType::Integer;
    result.integerValue = (int)pow(double(base.integerValue), double(exponent.integerValue));
    return result;
}

TValue GfxRequestSpriteDraw(TValue spriteId, TValue x, TValue y)
{
    ASSERT(spriteId.type == TValue::ValueType::Integer);
    ASSERT(x.type == TValue::ValueType::Integer || x.type == TValue::ValueType::Real);
    ASSERT(y.type == TValue::ValueType::Integer || y.type == TValue::ValueType::Real);
    float fx = float(x.type == TValue::ValueType::Integer ? x.integerValue : x.realValue);
    float fy = float(y.type == TValue::ValueType::Integer ? y.integerValue : y.realValue);

    Gfx::QueueSpriteForRender(spriteId.integerValue, vec2(fx, fy));

    TValue result;
    return result;
}

// TODO should be gfx.clear(color)
TValue GfxClearColor(TValue r, TValue g, TValue b, TValue a)
{
    ASSERT(r.type == TValue::ValueType::Integer || r.type == TValue::ValueType::Real);
    ASSERT(g.type == TValue::ValueType::Integer || g.type == TValue::ValueType::Real);
    ASSERT(b.type == TValue::ValueType::Integer || b.type == TValue::ValueType::Real);
    ASSERT(a.type == TValue::ValueType::Integer || a.type == TValue::ValueType::Real);
    float fr = float(r.type == TValue::ValueType::Integer ? r.integerValue : r.realValue);
    float fg = float(g.type == TValue::ValueType::Integer ? g.integerValue : g.realValue);
    float fb = float(b.type == TValue::ValueType::Integer ? b.integerValue : b.realValue);
    float fa = float(a.type == TValue::ValueType::Integer ? a.integerValue : a.realValue);

    Gfx::SetGameLayerClearColor(vec4(fr, fg, fb, fa));

    TValue result;
    return result;
}

void BindPipAPI()
{
    // TODO these need to go inside a map called gfx
    pipl_bind_cpp_fn("gfx_clear", 4, GfxClearColor);
    pipl_bind_cpp_fn("gfx_sprite", 3, GfxRequestSpriteDraw);


    pipl_bind_cpp_fn("raise_to", 2, Temp_RaiseTo);
}
