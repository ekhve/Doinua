/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once
#include <stdint.h>
#include "fm1kit.h"
int kit__obv_npresets(void);
const char *kit__obv_preset_name(int p);
void kit__obv_select(int t, int p);
int kit__obv_on(int t);
void kit__obv_note_on(int t, int note, int vel);
void kit__obv_note_off(int t, int note);
void kit__obv_all_off(int t);
int kit__obv_nactive(int t);
int kit__obv_nreleasing(int t);
int kit__obv_has(int t, int note);
int kit__obv_steal(int t);
void kit__obv_render(int t, int32_t *mix, int n);
void kit__obv_sends(int t, uint8_t sends[4]);
int kit__obv_preset_arp(int preset, kit_preset_arp_t *arp);
