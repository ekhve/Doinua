/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Charles Vestal
 * Copyright (C) 2026 John Brown */
/* fm1-scales: ported from imijes/bass-something-2 at 4c8a82f. */
#pragma once
#include <stdint.h>
typedef union { float f; uint32_t u; } fm_bits_t;
static inline float vm_fabsf(float x) { fm_bits_t b; b.f = x; b.u &= 0x7FFFFFFFu; return b.f; }
static inline float vm_minf(float a, float b) { return a < b ? a : b; }
static inline float vm_clampf(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }
static inline float vm_floorf(float x)
{
    const int32_t i = (int32_t)x;
    const float f = (float)i;
    return f > x ? f - 1.0f : f;
}
static inline float vm_exp2f(float x)       /* fm_exp2f's */
{
    fm_bits_t b;
    float fl, f, p;
    int32_t n;
    if (x < -126.0f)
        return 0.0f;
    if (x > 127.0f)
        x = 127.0f;
    fl = vm_floorf(x + 0.5f);
    n = (int32_t)fl, f = x - fl;
    if (n < -126)
        return 0.0f;
    p = 1.534580891e-04f;
    p = p * f + 1.339993158e-03f;
    p = p * f + 9.618488972e-03f;
    p = p * f + 5.550328776e-02f;
    p = p * f + 2.402264689e-01f;
    p = p * f + 6.931472057e-01f;
    p = p * f + 1.0f;
    b.f = p;
    b.u += (uint32_t)n << 23;
    return b.f;
}
static inline float vm_tanhf(float x)       /* fm_tanhf's */
{
    float a = vm_fabsf(x), e, r;
    if (a < 0.0004f)
        return x;
    if (a < 0.5f) {
        float z = x * x;
        return x * (1.0f + z * (-0.333333333f + z * (0.133333333f + z * (-0.0539682540f + z * (0.0218694885f +
                    z * (-0.00886323552f + z * 0.00359212803f))))));
    }
    if (a > 9.0f)
        return x > 0.0f ? 1.0f : -1.0f;
    e = vm_exp2f(2.0f * 1.44269504089f * a);
    r = 1.0f - 2.0f / (e + 1.0f);
    return x > 0.0f ? r : -r;
}

/* Positive gain-normalization factors. Two Newton steps keep this independent
 * of the host math library and the device runtime. */
static inline float bs2_rsqrt(float x)
{
    fm_bits_t bits = {.f = x};
    bits.u = 0x5f3759dfu - (bits.u >> 1);
    float y = bits.f;
    y *= 1.5f - 0.5f * x * y * y;
    return y * (1.5f - 0.5f * x * y * y);
}
