/* SPDX-License-Identifier: GPL-2.0-or-later; Max Seelig (MAXmade). */
#ifndef KALEIDO_CAPTURE_GATE_H
#define KALEIDO_CAPTURE_GATE_H
#include "fx_camera.h"
#define kaleido_gate_automatic FX(gate_automatic)
#define kaleido_gate_request FX(gate_request)
#define kaleido_gate_request_source FX(gate_request_source)
#define kaleido_gate_request_patch FX(gate_request_patch)
#define kaleido_gate_request_full FX(gate_request_full)
#define kaleido_gate_service FX(gate_service)
#define kaleido_gate_full_press FX(gate_full_press)
#define kaleido_gate_reader FX(gate_reader)
#define kaleido_gate_cancel FX(gate_cancel)
#define kaleido_gate_shutdown FX(gate_shutdown)
#define kaleido_gate_status FX(gate_status)
enum {MG_IDLE,MG_QUEUED,MG_ARMED,MG_HELD,MG_DONE,MG_FAILED};
enum {MG_OK,MG_NO_PREVIEW,MG_TRACE_BUSY,MG_TRACE_FAILED,MG_GUARD,MG_PATCH,
      MG_CONTEXT,MG_NO_SHUTTER,MG_READOUT,MG_OVERLAP,MG_CANCEL,MG_TIMEOUT,
      MG_OVERRUN,MG_LOG,MG_SHUTDOWN,MG_SOURCE,MG_PATCH_PREP,MG_PATCH_VERIFY,MG_CONTROLS};
typedef struct {
    unsigned phase,error,installed,core_error,requested_ms,armed_ms;
    unsigned latched,full_ms,preview_ms;fx_params params;
    unsigned enter_us,release_us,held_us,sleeps,cpsr,lr,task,context,source,job;
    unsigned dma_before,dma_after,dma_address,dma_pointer;
    unsigned overlaps,first_overlap_pc,first_overlap_us;
    unsigned sample_mode,sample_before_us,sample_after_us,before_count,after_count,changed;
    unsigned patch_started,patch_done,patch_us,patch_verified,patch_mismatches,patch_changed;
    char log[64];
    char sample_log[64];
} kaleido_gate_report;
/* Opt-in, one-photo, requested 100ms scheduling experiment. Default request
 * accesses no pixels; source mode only reads. Neither grants ownership/coherence. Original
 * SVC/IRQ/FIQ context, caller, readout and physical-edge latch are required.
 * Normal ML loader only; the hook remains resident until power OFF. */
/* Re-arm the guarded full-image fold after each completed single shot.
 * Uses install-only safety observers and writes no diagnostic files.
 * A fault stays latched until OFF/ON; held shutter never re-arms. */
void kaleido_gate_automatic(unsigned enabled);
int kaleido_gate_request(void);
/* Same guarded hold, plus two bounded uncached read-only sample passes.
 * No source writes/cache operations; samples must be compared with that CR2. */
int kaleido_gate_request_source(void);
/* Experimental ONE-shot 1024x512 sensor rectangle, latched Kaleido geometry.
 * Physical shutter only. Changes this photo; does not enable full-frame capture.
 * No allocator, file IO, global cache clean or persistent setting changes. */
int kaleido_gate_request_patch(void);
/* Experimental ONE-shot 5184x3456 active-area kaleidoscope.
 * Optical black/margins remain original. Two-second post-hoc hold budget,
 * not a watchdog guarantee; the complete transform finishes once started. */
int kaleido_gate_request_full(void);
void kaleido_gate_service(void);
void kaleido_gate_full_press(void);
void kaleido_gate_reader(unsigned pc);
void kaleido_gate_cancel(void);
void kaleido_gate_shutdown(void);
void kaleido_gate_status(kaleido_gate_report*out);
#endif
