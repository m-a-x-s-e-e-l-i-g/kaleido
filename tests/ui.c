/* Actual frontend; synthetic ML menu/worker/config/preview transport. */
#include <assert.h>
#include "../modules/kaleido/kaleido.c"
int lv=1,lv_paused,ml_shutdown_requested;
struct test_ui_lens lens_info;
static unsigned now,menu=1,half,starts,saves,autos,fail_save,fulls,off_during_start;
static int stopping,have_saved;
static kaleido_settings saved;
static fx_status report;
static fx_params requested;
static kaleido_gate_report gate_report;
unsigned get_ms_clock(void){return now;}
int get_halfshutter_pressed(void){return (int)half;}
int gui_menu_shown(void){return (int)menu;}
void msleep(unsigned ms){now+=ms;}
unsigned task_create(const char*a,unsigned b,unsigned c,void(*d)(void*),void*e)
{(void)a;(void)b;(void)c;(void)d;(void)e;return 1;}
int is_camera(const char*a,const char*b){return !strcmp(a,"EOSM")&&!strcmp(b,"2.0.2");}
void menu_add(const char*a,struct menu_entry*b,unsigned c){assert(!strcmp(a,"Shoot")&&b==entries&&c==1);}
void menu_remove(const char*a,struct menu_entry*b,unsigned c){(void)a;(void)b;(void)c;}
void fx_camera_init(void){memset(&report,0,sizeof(report));kaleido_active=stopping=0;}
int fx_native_capture_mode(void){return 1;}
int fx_capture_scratch_configure(size_t n){assert(n==895796);return 1;}
int fx_controls(const fx_params*p){assert(p->value[0]<360&&(p->value[1]==4||p->value[1]==6||p->value[1]==12));requested=*p;return 1;}
void fx_canon_gui(unsigned s){(void)s;}
void fx_half(int h){(void)h;}
unsigned fx_filter(void*p){(void)p;return 0;}
int fx_key(unsigned k){(void)k;return 1;}
int fx_start(void){assert(!kaleido_active);starts++;kaleido_active=1;stopping=0;if(off_during_start){off_during_start=0;toggle(0,1);}return 1;}
void fx_stop(void){stopping=1;}
void fx_service(void){if(stopping){kaleido_active=0;stopping=0;}report.running=(unsigned)kaleido_active;}
void fx_status_get(fx_status*out){*out=report;}
void kaleido_gate_automatic(unsigned on){autos=on;gate_report.phase=on?MG_ARMED:MG_IDLE;}
void kaleido_gate_full_press(void){fulls++;}
void kaleido_gate_status(kaleido_gate_report*out){*out=gate_report;}
void kaleido_gate_service(void){}
void kaleido_raw_trace_service(void){}
void kaleido_raw_trace_cancel(void){}
void kaleido_gate_shutdown(void){}
void kaleido_raw_trace_shutdown(void){}
int kaleido_settings_load(kaleido_settings*out){if(have_saved)*out=saved;return have_saved;}
int kaleido_settings_save(const kaleido_settings*s){saves++;if(fail_save)return 0;saved=*s;have_saved=1;return 1;}
int main(void)
{
    (void)sdk_module_info;(void)sdk_callbacks;(void)sdk_props;(void)sdk_configs;
    assert(!init());now=2000;service_once();assert(!starts&&!preferences.enabled);
    toggle(0,1);now+=500;service_once();assert(saved.enabled&&preferences.enabled&&!starts);
    menu=0;service_once();assert(starts==1&&kaleido_active&&autos);
    assert(key(3)==1&&fulls==1);
    struct menu_entry*children=entries[0].children;struct menu_display_info info={0};
    char expected[32];
    for(unsigned angle=0;angle<360;angle++){
        preferences.rotation=angle;number_status(&children[2],&info);
        snprintf(expected,sizeof(expected),"%u deg",angle);assert(!strcmp(info.value,expected));
    }
    for(unsigned center=0;center<=1000;center+=25){
        preferences.center_x=center;preferences.center_y=1000-center;
        number_status(&children[3],&info);snprintf(expected,sizeof(expected),"%u.%u%%",center/10,center%10);assert(!strcmp(info.value,expected));
        number_status(&children[4],&info);snprintf(expected,sizeof(expected),"%u.%u%%",(1000-center)/10,(1000-center)%10);assert(!strcmp(info.value,expected));
    }
    preferences.rotation=359;configure(&preferences.rotation,1);assert(!preferences.rotation);
    configure(&preferences.rotation,-1);assert(preferences.rotation==359);
    preferences.center_x=0;configure(&preferences.center_x,-1);assert(!preferences.center_x);
    preferences.center_x=1000;configure(&preferences.center_x,1);assert(preferences.center_x==1000);
    preferences.segments=2;status(entries,&info);assert(strstr(info.help,"12 segments"));
    gate_report.phase=MG_FAILED;gate_report.error=17;status(entries,&info);assert(strstr(info.help,"code 17"));gate_report.phase=MG_ARMED;
    now+=500;service_once();assert(!dirty&&have_saved);
    unsigned before=starts;shutting_down=1;fx_stop();service_once();worker(0);assert(saved.enabled&&!kaleido_active);
    assert(!init());assert(preferences.rotation==359&&preferences.center_x==1000&&preferences.segments==2);
    now+=1500;service_once();assert(starts==before+1&&autos);
    fail_save=1;toggle(0,1);now+=500;service_once();assert(save_error&&dirty);
    fail_save=0;now+=2000;service_once();assert(!dirty&&!save_error&&!saved.enabled);
    puts("Kaleido UI: 360 rotation labels, 82 percent labels, signed-only Canon formatter, wrapping/bounds, status/help, restart and persistence PASS");
    return 0;
}
