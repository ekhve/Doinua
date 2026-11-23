/* SPDX-License-Identifier: GPL-3.0-only */
/* OberVave a1f660c, obv_engine.h. Tick once per 16 audio frames. */
#pragma once
#include <stdint.h>
#define ENV_ONE (1 << 24)
#define LEVEL_FLOOR 2048                        /* -78 dB: a release below this frees the voice */

enum { E_IDLE, E_ATT, E_DEC, E_SUS, E_REL };
typedef struct { uint32_t stage; int32_t level; } env_t;   /* level Q24 */


static const uint32_t ENV_K_ATT[128] = {
    10986490, 10505448, 10024988, 9547864, 9076568, 8613319, 8160047, 7718404,
    7289765, 6875246, 6475717, 6091822, 5724001, 5372510, 5037443, 4718755,
    4416277, 4129740, 3858787, 3602996, 3361886, 3134937, 2921596, 2721291,
    2533435, 2357436, 2192706, 2038658, 1894718, 1760324, 1634933, 1518016,
    1409066, 1307597, 1213143, 1125262, 1043531, 967552, 896946, 831356,
    770446, 713897, 661413, 612713, 567536, 525635, 486781, 450760,
    417369, 386423, 357746, 331175, 306560, 283758, 262639, 243080,
    224968, 208196, 192668, 178292, 164983, 152663, 141259, 130703,
    120934, 111892, 103524, 95780, 88614, 81983, 75846, 70169,
    64915, 60054, 55556, 51395, 47545, 43983, 40687, 37639,
    34818, 32209, 29795, 27561, 25495, 23584, 21816, 20180,
    18667, 17268, 15973, 14775, 13667, 12642, 11694, 10817,
    10006, 9256, 8561, 7919, 7325, 6776, 6268, 5798,
    5363, 4961, 4588, 4244, 3926, 3631, 3359, 3107,
    2874, 2658, 2459, 2275, 2104, 1946, 1800, 1665,
    1540, 1425, 1318, 1219, 1128, 1043, 965, 892,
};
static const uint32_t ENV_K_DEC[128] = {
    14874750, 14537265, 14172024, 13781369, 13368054, 12935148, 12485928, 12023778,
    11552103, 11074244, 10593411, 10112632, 9634704, 9162169, 8697296, 8242069,
    7798190, 7367084, 6949913, 6547589, 6160798, 5790015, 5435527, 5097459,
    4775786, 4470363, 4180936, 3907164, 3648636, 3404881, 3175384, 2959599,
    2756954, 2566867, 2388745, 2221999, 2066042, 1920297, 1784200, 1657203,
    1538775, 1428406, 1325605, 1229903, 1140853, 1058029, 981027, 909466,
    842985, 781244, 723921, 670715, 621344, 575541, 533059, 493665,
    457141, 423284, 391904, 362825, 335881, 310919, 287796, 266379,
    246544, 228175, 211166, 195418, 180837, 167339, 154844, 143278,
    132572, 122663, 113493, 105005, 97151, 89883, 83157, 76933,
    71174, 65845, 60914, 56352, 52132, 48226, 44613, 41271,
    38178, 35317, 32670, 30222, 27957, 25861, 23922, 22129,
    20470, 18935, 17515, 16202, 14987, 13863, 12824, 11862,
    10973, 10150, 9388, 8684, 8033, 7431, 6873, 6358,
    5881, 5440, 5032, 4654, 4305, 3982, 3684, 3407,
    3152, 2915, 2697, 2494, 2307, 2134, 1974, 1826,
};

/* Signed level difference times a positive Q24 coefficient. The 12-bit
 * halves retain the low-product carry without a 64-bit device multiply. */
static inline int32_t obv_env_scale(int32_t difference, int32_t coefficient)
{
    int32_t hi = difference >> 12, lo = difference & 4095;
    int32_t kh = coefficient >> 12, kl = coefficient & 4095;
    return hi * kh + ((hi * kl + lo * kh + ((lo * kl) >> 12)) >> 12);
}
/* ---- envelopes: analog RC curves per block (CTL frames), level Q24 */
static inline void env_tick(env_t *e, uint32_t a, uint32_t d, int32_t sus, uint32_t r, int32_t gate, int32_t vint)
{
    int32_t k, target;
    if (gate && e->stage >= E_REL) e->stage = E_ATT;
    if (!gate && e->stage && e->stage < E_REL) e->stage = E_REL;
    switch (e->stage) {
    case E_ATT:
        k = (int32_t)ENV_K_ATT[a];
        k += (k >> 7) * vint + (((k & 127) * vint) >> 7);
        target = ENV_ONE + (ENV_ONE * 3 >> 3);  /* aims at 1.3, stops at 1.0 */
        e->level += obv_env_scale(target - e->level, k);
        if (e->level >= ENV_ONE) { e->level = ENV_ONE; e->stage = E_DEC; }
        break;
    case E_DEC:
        k = (int32_t)ENV_K_DEC[d];
        k += (k >> 7) * vint + (((k & 127) * vint) >> 7);
        e->level += obv_env_scale(sus - e->level, k) - 16;
        if (e->level <= sus + 64) { e->level = sus; e->stage = E_SUS; }
        break;
    case E_SUS:
        e->level = sus;                         /* the knob moves it */
        break;
    case E_REL:
        k = (int32_t)ENV_K_DEC[r];
        k += (k >> 7) * vint + (((k & 127) * vint) >> 7);
        e->level += obv_env_scale(0 - e->level, k) - 32;
        if (e->level < LEVEL_FLOOR) { e->level = 0; e->stage = E_IDLE; }
        break;
    default:
        e->level = 0;
    }
}

