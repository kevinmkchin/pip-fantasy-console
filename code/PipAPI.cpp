#include "PipAPI.h"

#include "MesaScript.h"
#include "GfxRenderer.h"


MesaTValue Temp_RaiseTo(MesaTValue base, MesaTValue exponent)
{
    ASSERT(base.type == MesaTValue::ValueType::Integer);
    ASSERT(exponent.type == MesaTValue::ValueType::Integer);

    MesaTValue result;
    result.type = MesaTValue::ValueType::Integer;
    result.integerValue = (int)pow(double(base.integerValue), double(exponent.integerValue));
    return result;
}

MesaTValue GfxRequestSpriteDraw(MesaTValue spriteId, MesaTValue x, MesaTValue y)
{
    ASSERT(spriteId.type == MesaTValue::ValueType::Integer);
    ASSERT(x.type == MesaTValue::ValueType::Integer || x.type == MesaTValue::ValueType::Real);
    ASSERT(y.type == MesaTValue::ValueType::Integer || y.type == MesaTValue::ValueType::Real);
    float fx = float(x.type == MesaTValue::ValueType::Integer ? x.integerValue : x.realValue);
    float fy = float(y.type == MesaTValue::ValueType::Integer ? y.integerValue : y.realValue);

    Gfx::QueueSpriteForRender(spriteId.integerValue, vec2(fx, fy));

    MesaTValue result;
    return result;
}

// TODO should be gfx.clear(color)
MesaTValue GfxClearColor(MesaTValue r, MesaTValue g, MesaTValue b, MesaTValue a)
{
    ASSERT(r.type == MesaTValue::ValueType::Integer || r.type == MesaTValue::ValueType::Real);
    ASSERT(g.type == MesaTValue::ValueType::Integer || g.type == MesaTValue::ValueType::Real);
    ASSERT(b.type == MesaTValue::ValueType::Integer || b.type == MesaTValue::ValueType::Real);
    ASSERT(a.type == MesaTValue::ValueType::Integer || a.type == MesaTValue::ValueType::Real);
    float fr = float(r.type == MesaTValue::ValueType::Integer ? r.integerValue : r.realValue);
    float fg = float(g.type == MesaTValue::ValueType::Integer ? g.integerValue : g.realValue);
    float fb = float(b.type == MesaTValue::ValueType::Integer ? b.integerValue : b.realValue);
    float fa = float(a.type == MesaTValue::ValueType::Integer ? a.integerValue : a.realValue);

    Gfx::SetGameLayerClearColor(vec4(fr, fg, fb, fa));

    MesaTValue result;
    return result;
}

void BindPipAPI()
{
    // TODO these need to go inside a map called gfx
    pipl_bind_cpp_fn("gfx_clear", 4, GfxClearColor);
    pipl_bind_cpp_fn("gfx_sprite", 3, GfxRequestSpriteDraw);


    pipl_bind_cpp_fn("raise_to", 2, Temp_RaiseTo);
}
