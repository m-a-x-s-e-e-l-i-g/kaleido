#ifndef MIRROR_UI_SDK_H
#define MIRROR_UI_SDK_H
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
static inline int sdk_menu_snprintf(char*out,size_t n,const char*fmt,...)
{
    for(const char*p=fmt;*p;p++)if(*p=='%'){p++;assert(*p=='d'||*p=='s'||*p=='%');}
    va_list ap;va_start(ap,fmt);int result=vsnprintf(out,n,fmt,ap);va_end(ap);return result;
}
#include <string.h>
#define COUNT(a) (sizeof(a)/sizeof((a)[0]))
#define CONFIG_INT(name,var,value) int var=value
#define MENU_SELECT_FUNC(f) void f(void*priv,int delta)
#define MENU_UPDATE_FUNC(f) void f(struct menu_entry*entry,struct menu_display_info*info)
struct menu_display_info{char value[128],help[256];};
struct menu_entry{const char*name;void*priv;int max;void(*select)(void*,int);void(*update)(struct menu_entry*,struct menu_display_info*);const char*help;struct menu_entry*children;const char**choices;};
#define MENU_SET_VALUE(...) sdk_menu_snprintf(info->value,sizeof(info->value),__VA_ARGS__)
#define MENU_SET_HELP(...) sdk_menu_snprintf(info->help,sizeof(info->help),__VA_ARGS__)
#define MENU_EOL {0}
#define PROP_HANDLER(p) void prop_##p(void*buf,unsigned len)
#define MODULE_INFO_START() static unsigned(*const sdk_module_info[])(void)={
#define MODULE_LONGNAME(x)
#define MODULE_INIT(f) f,
#define MODULE_DEINIT(f) f,
#define MODULE_INFO_END() };
#define MODULE_CBRS_START() static unsigned(*const sdk_callbacks[])(unsigned)={
#define MODULE_CBR(kind,f,unused) f,
#define MODULE_CBRS_END() };
#define MODULE_PROPHANDLERS_START() static void(*const sdk_props[])(void*,unsigned)={
#define MODULE_PROPHANDLER(p) prop_##p,
#define MODULE_PROPHANDLERS_END() };
#define MODULE_CONFIGS_START() static int*const sdk_configs[]={
#define MODULE_CONFIG(v) &v,
#define MODULE_CONFIGS_END() };
extern int lv,lv_paused,ml_shutdown_requested;
extern int gui_state,shooting_mode;
extern struct test_ui_lens{int job_state;}lens_info;
unsigned get_ms_clock(void);
int get_halfshutter_pressed(void);
int gui_menu_shown(void);
int is_movie_mode(void);
void msleep(unsigned ms);
unsigned task_create(const char*,unsigned,unsigned,void(*)(void*),void*);
int is_camera(const char*,const char*);
void menu_add(const char*,struct menu_entry*,unsigned);
void menu_remove(const char*,struct menu_entry*,unsigned);
#endif
