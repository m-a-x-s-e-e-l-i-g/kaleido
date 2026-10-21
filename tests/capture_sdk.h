/* Host-only transport/DMA simulator. Production compiles against the ML SDK. */
#ifndef GL_CAPTURE_TEST_SDK_H
#define GL_CAPTURE_TEST_SDK_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
/* Model the EOS M RAW-capable target rather than raw.h's unsupported stub. */
#define CONFIG_RAW_LIVEVIEW 1
#define CACHEABLE(p) (p)
#define UNCACHEABLE(p) (p)
#define SHOOTMODE_M 3
#define SHOOTMODE_AP 22
#define SHOOTMODE_P 0
#define SHOOTMODE_AV 2
#define WB_CUSTOM 6
#define RECORDING (__recording || __recording_custom)
extern int lv, lv_paused, lv_dispsize, shooting_mode, __recording, __recording_custom;
extern int sensor_cleaning, ml_shutdown_requested, icu_uilock, gui_state;
#define UILOCK_NONE 0
#define UILOCK_EVERYTHING 0x17f
#define UILOCK_POWER_SW 0x20
void PauseLiveView(void);
int ResumeLiveView(void);
void gui_uilock(int what);
int is_intervalometer_running(void);
int can_use_raw_overlays_photo(void);
void raw_set_dirty(void);
int raw_lv_is_enabled(void);
intptr_t call(const char *name, ...);
extern uint32_t raw_write_chan;
struct test_lens {
    unsigned job_state, raw_iso, raw_shutter, raw_aperture, focal_len, wb_mode;
    unsigned WBGain_R, WBGain_G, WBGain_B;
    int wbs_gm, wbs_ba;
};
extern struct test_lens lens_info;
struct config_var { const char *name; int *value; };
struct config_var *module_get_config_var(const char *name);
uint32_t cli(void);
void sei(uint32_t irq);
void *fio_malloc(size_t size);
void fio_free(void *memory);
void sync_caches(void);
int is_movie_mode(void);
int get_halfshutter_pressed(void);
int get_frame_iso(void);
int get_frame_aperture(void);
int get_current_shutter_reciprocal_x1000(void);
int raw2iso(int value);
int test_aperture_value(int raw);
int test_shutter_value(int raw);
#define RAW2VALUE(param, raw) test_##param##_value(raw)
void LoadCalendarFromRTC(struct tm *time);
uint32_t get_ms_clock(void);
void msleep(unsigned time);
uint32_t edmac_get_state(uint32_t channel);
uintptr_t edmac_get_address(uint32_t channel);
uintptr_t edmac_get_pointer(uint32_t channel);
int is_dir(const char *path);
int FIO_CreateDirectory(const char *path);
int FIO_GetFileSize(const char *path, uint32_t *size);
FILE *FIO_CreateFile(const char *path);
int FIO_RemoveFile(const char *path);
void FIO_CloseFile(FILE *file);
int FIO_WriteFile(FILE *file, const void *data, size_t size);
struct display_filter_buffers { void *src_buf; void *dst_buf; };
struct vram_info { unsigned char *vram; int width, pitch, height; };
extern struct vram_info vram_lv;
extern int ext_monitor_hdmi;
struct vram_info *get_yuv422_vram(void);
int gui_menu_shown(void);
void gl_test_display_copy(void *dst, const void *src, size_t size);
unsigned int gl_test_live_filter(void *context);
void NotifyBox(int timeout, char *fmt, ...);
/* Exact portable key numbers from pinned src/module.h; EOS M mapping still
 * requires hardware observation. No fake zoom/full-shutter events here. */
#define MODULE_KEY_PRESS_HALFSHUTTER 1
#define MODULE_KEY_WHEEL_LEFT 7
#define MODULE_KEY_WHEEL_RIGHT 8
#define MODULE_KEY_PRESS_SET 9
#define MODULE_KEY_UNPRESS_SET 10
#define MODULE_KEY_PRESS_UP 12
#define MODULE_KEY_PRESS_RIGHT 15
#define MODULE_KEY_PRESS_LEFT 16
#define MODULE_KEY_PRESS_DOWN 19
#define MODULE_KEY_UNPRESS_UDLR 20
#define MODULE_KEY_MENU 22
#define MODULE_KEY_PLAY 24
#define BGMT_UNPRESS_LEFT 0x29
#define BGMT_UNPRESS_RIGHT 0x27
#define BGMT_UNPRESS_UP 0x2b
#define BGMT_UNPRESS_DOWN 0x2d
#endif
