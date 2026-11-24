/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
int kit__bs2_npresets(void);
const char *kit__bs2_preset_name(int p);
void kit__bs2_select(int t, int p);
int kit__bs2_on(int t);
void kit__bs2_note_on(int t, int note, int vel);
void kit__bs2_note_off(int t, int note);
void kit__bs2_all_off(int t);
int kit__bs2_nactive(int t);
int kit__bs2_nreleasing(int t);
int kit__bs2_has(int t, int note);
int kit__bs2_steal(int t);
void kit__bs2_render(int t, int32_t *mix, int n);
