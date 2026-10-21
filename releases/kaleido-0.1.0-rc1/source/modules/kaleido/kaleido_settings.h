/* SPDX-License-Identifier: GPL-2.0-or-later; Max Seelig (MAXmade). */
#ifndef KALEIDO_SETTINGS_H
#define KALEIDO_SETTINGS_H
#include "fx_mirror.h"
#define kaleido_settings_load FX(settings_load)
#define kaleido_settings_save FX(settings_save)
typedef struct {unsigned enabled,segments,rotation,center_x,center_y;} kaleido_settings;
static inline unsigned kaleido_segment_count(unsigned index)
{static const unsigned counts[]={4,6,12};return index<3?counts[index]:0;}
static inline fx_params kaleido_settings_params(const kaleido_settings*s)
{fx_params p={{s->rotation,kaleido_segment_count(s->segments),s->center_x,s->center_y}};return p;}
static inline int kaleido_settings_valid(const kaleido_settings*s)
{return s&&s->enabled<=1&&s->segments<3&&s->rotation<360&&s->center_x<=1000&&s->center_y<=1000;}
/* Checked alternating slots; only called from idle worker/init tasks. */
int kaleido_settings_load(kaleido_settings*out);
int kaleido_settings_save(const kaleido_settings*settings);
#endif
