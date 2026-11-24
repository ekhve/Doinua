/* SPDX-License-Identifier: GPL-3.0-only */
/* OberVave a1f660c glide at the original 16-frame control interval. */
#pragma once
#include <stdint.h>

static const uint16_t obv_glide_coefficient[128] = {
    12820, 12209, 11624, 11064, 10528, 10016, 9526, 9059, 8612, 8186,
    7779, 7392, 7022, 6670, 6335, 6015, 5711, 5422, 5146, 4884,
    4635, 4398, 4173, 3959, 3755, 3562, 3378, 3204, 3038, 2881,
    2732, 2590, 2456, 2328, 2207, 2092, 1983, 1879, 1781, 1688,
    1600, 1516, 1437, 1361, 1290, 1222, 1158, 1098, 1040, 985,
    933, 884, 838, 794, 752, 712, 675, 639, 606, 574,
    543, 515, 488, 462, 438, 414, 393, 372, 352, 334,
    316, 299, 284, 269, 254, 241, 228, 216, 205, 194,
    184, 174, 165, 156, 148, 140, 133, 126, 119, 113,
    107, 101, 96, 91, 86, 81, 77, 73, 69, 65,
    62, 59, 56, 53, 50, 47, 45, 42, 40, 38,
    36, 34, 32, 31, 29, 27, 26, 25, 23, 22,
    21, 20, 19, 18, 17, 16, 15, 14,
};

/* Move pitch toward target in Q16 semitones. Split the product into
 * 16-bit limbs to retain the upstream rounding on the FM-1. */
static inline int32_t obv_glide_tick(int32_t pitch, int32_t target, uint32_t coefficient)
{
    int32_t difference = target - pitch;
    if (!coefficient || (difference > -64 && difference < 64)) return target;
    int32_t whole = (difference >> 16) * (int32_t)coefficient;
    uint32_t fraction = ((uint32_t)difference & 65535u) * coefficient;
    return pitch + whole + (int32_t)(fraction >> 16);
}
