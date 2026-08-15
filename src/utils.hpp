#pragma once

#include "ZunMath.hpp"
#include "inttypes.hpp"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

union AnyArg {
    i32 i;
    u32 u;
    f32 f;
    i16 s[2];
    u16 us[2];
    i8 c[4];
    u8 b[4];
};
static_assert(sizeof(AnyArg) == 4);

namespace utils
{
inline f32 AddNormalizeAngle(f32 param_1, f32 param_2)
{
    i32 local_8;

    local_8 = 0;
    param_1 += param_2;
    while (param_1 > ZUN_PI)
    {
        param_1 -= ZUN_2PI;
        if (local_8++ > 16)
        {
            break;
        }
    }
    while (param_1 < -ZUN_PI)
    {
        param_1 += ZUN_2PI;
        if (local_8++ > 16)
        {
            break;
        }
    }
    return param_1;
}

inline f32 Lerp(f32 a, f32 b, f32 t)
{
    return a + (b - a) * t;
}

inline f32 LerpAngle(f32 from, f32 to, f32 alpha)
{
    return from + utils::AddNormalizeAngle(to - from, 0.0f) * alpha;
}

void Rotate(ZunVec3 *out, ZunVec3 *point, f32 angle);
} // namespace utils
