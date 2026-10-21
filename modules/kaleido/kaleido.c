/* SPDX-License-Identifier: GPL-2.0-or-later; Max Seelig (MAXmade). */
#ifdef FX_MIRROR_UI_HOST_TEST
#include "mirror_ui_sdk.h"
#else
#include <module.h>
#include <dryos.h>
#include <menu.h>
#include <property.h>
#include <config.h>
#include <lens.h>
#endif
#include "fx_camera.h"
#include "effect_config.h"
#include "kaleido_raw_trace.h"
#include "kaleido_capture_gate.h"
#include "kaleido_settings.h"
#include <string.h>
CONFIG_INT("maxmade.fx.kaleido.busy",kaleido_active,0);
static volatile int shutting_down,finished;
static kaleido_settings preferences={0,1,0,500,500};
static unsigned attempted,started_ms,dirty,changed_ms,save_error;
static const char*segments[]={"4","6","12"};
static struct menu_entry entries[];
static void service_once(void)
{
    unsigned now=get_ms_clock();
    if(!preferences.enabled||shutting_down||ml_shutdown_requested)fx_stop();
    if(preferences.enabled&&!attempted&&!kaleido_active&&!shutting_down&&!ml_shutdown_requested&&
       now-started_ms>=1500&&!lens_info.job_state&&lv&&!lv_paused&&!get_halfshutter_pressed()&&!gui_menu_shown()){
        attempted=1;
        if(fx_start())kaleido_gate_automatic(1);
    }
    /* A menu OFF arriving while startup resolves the module family wins. */
    if(!preferences.enabled)fx_stop();
    fx_service();
    if(!kaleido_active){kaleido_gate_automatic(0);kaleido_raw_trace_cancel();}
    kaleido_raw_trace_service();kaleido_gate_service();
    kaleido_gate_report capture;kaleido_gate_status(&capture);
    if(kaleido_active&&capture.phase==MG_FAILED)fx_stop();
    if(dirty&&!lens_info.job_state&&capture.phase!=MG_HELD&&!get_halfshutter_pressed()&&
       (shutting_down||ml_shutdown_requested||now-changed_ms>=(save_error?2000u:400u))){
        kaleido_settings snapshot=preferences;
        if(kaleido_settings_save(&snapshot)){
            if(!memcmp(&preferences,&snapshot,sizeof(snapshot)))dirty=0;
            save_error=0;
        }else{save_error=1;changed_ms=now;}
    }
}
static void worker(void*unused)
{
    while(!shutting_down||kaleido_active){service_once();msleep(20);}
    /* Persist the desired ON/OFF state, never the transient shutdown state. */
    if(dirty)kaleido_settings_save(&preferences);
    finished=1;
}
PROP_HANDLER(PROP_HALF_SHUTTER){if(buf&&len)fx_half(((unsigned char*)buf)[0]!=0);}
PROP_HANDLER(PROP_GUI_STATE){if(buf&&len>=4)fx_canon_gui(((unsigned*)buf)[0]);}
static unsigned int filter(unsigned int ctx){return fx_filter((void*)(uintptr_t)ctx);}
static unsigned int key(unsigned int value)
{
    if(value==3u)kaleido_gate_full_press();
    return fx_key(value); /* Physical shutter, SET and Play remain Canon controls. */
}
static void settings_changed(void)
{
    fx_params p=kaleido_settings_params(&preferences);fx_controls(&p);
    /* A failed allocation is otherwise latched by attempted. An explicit
     * settings change may retry once after the menu closes; never busy-retry
     * memory failures or clear a latched capture fault. */
    if(preferences.enabled&&!kaleido_active){
        fx_status r;fx_status_get(&r);kaleido_gate_report capture;kaleido_gate_status(&capture);
        if(r.error==10&&capture.phase!=MG_FAILED)attempted=0;
    }
    dirty=1;changed_ms=get_ms_clock();
}
static MENU_SELECT_FUNC(toggle)
{
    preferences.enabled=!preferences.enabled;attempted=0;
    if(!preferences.enabled){kaleido_gate_automatic(0);kaleido_raw_trace_cancel();fx_stop();}
    settings_changed();
}
static MENU_SELECT_FUNC(configure)
{
    int step=delta<0?-1:1;
    unsigned*value=priv;
    int next;
    if(value==&preferences.segments){next=(int)*value+step;if(next<0)next=2;if(next>2)next=0;}
    else if(value==&preferences.rotation){next=(int)*value+step;if(next<0)next=359;if(next>359)next=0;}
    else{next=(int)*value+step*25;if(next<0)next=0;if(next>1000)next=1000;}
    *value=(unsigned)next;settings_changed();
}
static MENU_UPDATE_FUNC(number_status)
{
    unsigned*value=entry->priv;
    /* Pinned Canon vsnprintf supports signed decimal, not %u. */
    if(value==&preferences.rotation)MENU_SET_VALUE("%d deg",(int)*value);
    else MENU_SET_VALUE("%d.%d%%",(int)(*value/10),(int)(*value%10));
}
static const char*reason(unsigned code)
{
    switch(code){case 10:return "Not enough memory";case 11:return "Disable other modules";
    case 12:return "Native 1x LCD required";case 13:return "Image processing";
    case 14:return "No display delivery";case 16:return "Cancelled";default:return "Preview unavailable";}
}
static MENU_UPDATE_FUNC(status)
{
    fx_status r;fx_status_get(&r);kaleido_gate_report capture;kaleido_gate_status(&capture);
    if(capture.phase==MG_FAILED&&capture.error!=MG_CANCEL&&capture.error!=MG_SHUTDOWN){
        MENU_SET_VALUE("Capture unavailable");
        MENU_SET_HELP("%s. Capture stopped (code %d). Switch OFF/ON to retry; restart if it persists.",FX_VERSION,(int)capture.error);
    }else if(r.error){MENU_SET_VALUE("%s",reason(r.error));
        if(r.error==10)MENU_SET_HELP("%s. Startup free %d KB; need %d KB. Change a setting or switch OFF/ON, then close the menu to retry.",
            FX_VERSION,(int)(r.free_initial/1024),(int)(r.required/1024));
        else MENU_SET_HELP("%s. A+ / Auto ISO, Canon RAW, native 1x LCD and Global Draw ON.",FX_VERSION);
    }else{
        MENU_SET_VALUE("%s",!r.running?(preferences.enabled?"ON - Preparing":"OFF"):capture.phase==MG_ARMED?"ON - Ready":capture.phase==MG_HELD?"ON - Saving":"ON - Preparing");
        MENU_SET_HELP("%s. %d segments. Physical shutter saves kaleidoscope CR2; release between shots.",FX_VERSION,(int)kaleido_segment_count(preferences.segments));
    }
    if(save_error)MENU_SET_HELP("Could not save preferences. Check card write access; settings remain active this session.");
}
static struct menu_entry entries[]={
    {.name="Kaleido",.priv=&preferences.enabled,.max=1,.select=toggle,.update=status,
     .help=FX_VERSION " / Kaleidoscope preview and Canon RAW. Development candidate.",
     .children=(struct menu_entry[]){
        {.name="Live effect",.priv=&preferences.enabled,.max=1,.select=toggle,.update=status},
        {.name="Segments",.priv=&preferences.segments,.max=2,.select=configure,.choices=segments,
         .help="Repeat one wedge around the center: 4, 6 or 12 segments."},
        {.name="Rotation",.priv=&preferences.rotation,.max=359,.select=configure,.update=number_status,
         .help="Rotate the source wedge. One degree per step."},
        {.name="Center X",.priv=&preferences.center_x,.max=1000,.select=configure,.update=number_status,
         .help="Horizontal center. 2.5 percent per step; 50 percent is centered."},
        {.name="Center Y",.priv=&preferences.center_y,.max=1000,.select=configure,.update=number_status,
         .help="Vertical center. 2.5 percent per step; 50 percent is centered."},
        MENU_EOL,
     }},
};
static unsigned int init(void)
{
    if(!is_camera("EOSM","2.0.2"))return 1;
    fx_camera_init();if(!fx_native_capture_mode()||!fx_capture_scratch_prefer((2592u*1728u+4u)/5u,2u*1024u*1024u)||
        !fx_capture_scratch_share_workspace())return 1;
    kaleido_settings_load(&preferences);
    fx_params p=kaleido_settings_params(&preferences);fx_controls(&p);
    fx_canon_gui(0);shutting_down=finished=0;attempted=dirty=save_error=0;started_ms=get_ms_clock();
    unsigned task=task_create("kaleido_worker",0x1e,0x4000,worker,0);
    if(!task||task==0xffffffff)return 1;
    menu_add("Shoot",entries,COUNT(entries));return 0;
}
static unsigned int deinit(void)
{
    kaleido_gate_shutdown();kaleido_raw_trace_shutdown();shutting_down=1;fx_stop();
    kaleido_gate_report capture;
    for(unsigned i=0;i<600;i++){
        kaleido_gate_status(&capture);
        if(finished&&capture.phase!=MG_HELD)break;
        msleep(5);
    }
    kaleido_gate_status(&capture);if(!finished||capture.phase==MG_HELD)return 1;
    menu_remove("Shoot",entries,COUNT(entries));return 0;
}
MODULE_INFO_START() MODULE_LONGNAME("Kaleido") MODULE_INIT(init) MODULE_DEINIT(deinit) MODULE_INFO_END()
MODULE_CBRS_START()
    MODULE_CBR(CBR_DISPLAY_FILTER,filter,0)
    MODULE_CBR(CBR_KEYPRESS,key,0)
MODULE_CBRS_END()
MODULE_PROPHANDLERS_START()
    MODULE_PROPHANDLER(PROP_HALF_SHUTTER) MODULE_PROPHANDLER(PROP_GUI_STATE)
MODULE_PROPHANDLERS_END()
MODULE_CONFIGS_START() MODULE_CONFIG(kaleido_active) MODULE_CONFIGS_END()
