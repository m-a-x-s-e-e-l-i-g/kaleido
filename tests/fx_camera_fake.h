/* Real effect adapter; synthetic SDK, source pixels, clock and files. */
#include "fx_sdk.h"
#include "fx_camera.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
int FX(active);
int lv=1,lv_paused,lv_dispsize=1,ext_monitor_hdmi,sensor_cleaning,ml_shutdown_requested;
struct vram_info vram_lv={0,720,1440,480};
static unsigned now=100,irq_depth,in_filter,half,menu,allocation_count,fail_allocation,allocated,free_bytes=16*1024*1024;
static unsigned cpu_free_bytes=8*1024*1024,cpu_allocated,shared_heap,memory_overhead;
static unsigned stop_on_allocation,shutdown_on_allocation;
static int loaded_old,loaded_peer,peer_busy,foreign_filter,fail_write,fail_stat,stop_on_write,hook;
static void (*during_write)(void),(*during_sync)(void);
static void (*during_filter_sync)(void);
static void (*during_commit)(void);
static unsigned zoom_triggered,zoom_clear_calls,filter_syncs;
void *memcpy64(void*dest,void*src,size_t size){assert(!(((uintptr_t)dest|(uintptr_t)src)&3)&&!(size&7));return memcpy(dest,src,size);}
static unsigned char source[720*480*2],output[sizeof(source)],before[sizeof(source)],last[sizeof(source)];
static struct {char name[80];FILE*f;int closed,removed;} files[128];static unsigned file_count;
#ifdef FX_MIRROR_TEST
#define PEER_NAME "slitscan"
#else
#define PEER_NAME "mirror"
#endif
static struct config_var peer={"maxmade.fx." PEER_NAME ".busy",&peer_busy};
static struct {void*p;size_t bytes,charge;unsigned cpu;} blocks[8];
unsigned cli(void){return irq_depth++;}
void sei(unsigned previous){assert(irq_depth==previous+1);irq_depth=previous;}
uint32_t get_ms_clock(void){return now;}
int gui_menu_shown(void){return menu;}
int get_halfshutter_pressed(void){return half;}
struct vram_info*get_yuv422_vram(void){assert(!in_filter&&!irq_depth);return &vram_lv;}
int module_get_next_loaded(int i){assert(!irq_depth);return i<0&&(loaded_old||loaded_peer)?0:-1;}
const char*module_get_name(int i){assert(i==0);return loaded_old?"glock":PEER_NAME;}
struct config_var*module_get_config_var(const char*n){assert(!irq_depth);return loaded_peer&&!strcmp(n,peer.name)?&peer:0;}
int module_display_filter_enabled(void){return foreign_filter;}
int should_draw_zoom_overlay(void){assert(!irq_depth);return zoom_triggered!=0;}
int zebra_should_run(void){assert(!irq_depth);return !menu;}
void zoom_overlay_disable(void){assert(!irq_depth);zoom_triggered=0;zoom_clear_calls++;}
void fx_test_committed_canvas(void){assert(in_filter&&!irq_depth);if(during_commit){void(*f)(void)=during_commit;during_commit=0;f();}}
int GetFreeMemForAllocateMemory(void){return (int)free_bytes;}
int GetFreeMemForMalloc(void){return (int)cpu_free_bytes;}
static void*private_test_alloc(size_t bytes,unsigned cpu)
{
    assert(!irq_depth&&!in_filter);allocation_count++;
    if(fail_allocation==allocation_count)return 0;
    size_t charge=bytes+memory_overhead;
    if((cpu?cpu_free_bytes:free_bytes)<charge)return 0;
    if(shared_heap&&(cpu?free_bytes:cpu_free_bytes)<charge)return 0;
    void*p=malloc(bytes);assert(p);allocated+=(unsigned)bytes;
    if(cpu){cpu_free_bytes-=(unsigned)charge;cpu_allocated+=(unsigned)bytes;if(shared_heap)free_bytes-=(unsigned)charge;}
    else{free_bytes-=(unsigned)charge;if(shared_heap)cpu_free_bytes-=(unsigned)charge;}
    unsigned i=0;while(i<8&&blocks[i].p)i++;assert(i<8);
    blocks[i].p=p;blocks[i].bytes=bytes;blocks[i].charge=charge;blocks[i].cpu=cpu;
    if(stop_on_allocation==allocation_count)fx_stop();
    if(shutdown_on_allocation==allocation_count)ml_shutdown_requested=1;
    return p;
}
void*__priv_alloc_dma_memory(size_t bytes){return private_test_alloc(bytes,0);}
void*__priv_malloc(size_t bytes){return private_test_alloc(bytes,1);}
static void private_test_free(void*p,unsigned cpu)
{
    assert(!irq_depth&&!in_filter&&p);unsigned i=0;while(i<8&&blocks[i].p!=p)i++;assert(i<8&&blocks[i].cpu==cpu);
    allocated-=(unsigned)blocks[i].bytes;
    if(cpu){cpu_allocated-=(unsigned)blocks[i].bytes;cpu_free_bytes+=(unsigned)blocks[i].charge;if(shared_heap)free_bytes+=(unsigned)blocks[i].charge;}
    else{free_bytes+=(unsigned)blocks[i].charge;if(shared_heap)cpu_free_bytes+=(unsigned)blocks[i].charge;}
    blocks[i].p=0;free(p);
}
void __priv_free_dma_memory(void*p){private_test_free(p,0);}
void __priv_free(void*p){private_test_free(p,1);}
void sync_caches(void)
{
    assert(!irq_depth);
    if(in_filter)filter_syncs++;
    if(!in_filter&&during_sync){void(*f)(void)=during_sync;during_sync=0;f();}
    if(in_filter&&during_filter_sync){void(*f)(void)=during_filter_sync;during_filter_sync=0;f();}
    if(in_filter&&hook){int h=hook;hook=0;if(h==1){half=1;fx_half(1);}else if(h==2)assert(fx_capture());else fx_stop();}
}
static int find(const char*n){for(unsigned i=0;i<file_count;i++)if(!files[i].removed&&!strcmp(files[i].name,n))return (int)i;return -1;}
int is_dir(const char*n){(void)n;assert(!in_filter);return 1;}
int FIO_CreateDirectory(const char*n){(void)n;assert(!in_filter);return 0;}
int FIO_GetFileSize(const char*n,uint32_t*size)
{
    assert(!in_filter&&!irq_depth);int i=find(n);if(i<0)return -1;
    long old=ftell(files[i].f);fseek(files[i].f,0,SEEK_END);*size=(uint32_t)ftell(files[i].f);fseek(files[i].f,old,SEEK_SET);
    if(fail_stat&&files[i].closed&&strstr(n,".TIF"))(*size)--;
    return 0;
}
FILE*FIO_CreateFile(const char*n)
{
    assert(!in_filter&&!irq_depth&&find(n)<0&&file_count<128);
    unsigned i=file_count++;strcpy(files[i].name,n);files[i].f=tmpfile();assert(files[i].f);return files[i].f;
}
static int index_of(FILE*f){for(unsigned i=0;i<file_count;i++)if(files[i].f==f)return (int)i;abort();}
int FIO_WriteFile(FILE*f,const void*p,size_t n)
{
    assert(!in_filter&&!irq_depth);int i=index_of(f);assert(!files[i].closed);
    if(during_write){void(*f)(void)=during_write;during_write=0;f();}
    if(stop_on_write){stop_on_write=0;fx_stop();}
    if((fail_write==1&&strstr(files[i].name,".TIF"))||(fail_write==2&&strstr(files[i].name,".JSN"))||(fail_write==3&&strstr(files[i].name,".OK")))return 0;
    return (int)fwrite(p,1,n,f);
}
void FIO_CloseFile(FILE*f){assert(!in_filter&&!irq_depth);int i=index_of(f);assert(!files[i].closed);fflush(f);files[i].closed=1;}
int FIO_RemoveFile(const char*n){assert(!in_filter&&!irq_depth);int i=find(n);assert(i>=0&&files[i].closed);files[i].removed=1;return 0;}
static fx_status status(void){fx_status r;fx_status_get(&r);return r;}
static void frame(unsigned value)
{
    now+=100;
    for(unsigned i=0;i<sizeof(source);i+=4){source[i]=source[i+2]=0;source[i+1]=source[i+3]=(unsigned char)value;}
    memcpy(before,source,sizeof(source));struct display_filter_buffers b={source,output};
    in_filter=1;fx_filter(&b);in_filter=0;
    assert(!memcmp(before,source,sizeof(source)));
}
static void dump(const char*name,const char*dest)
{
    int i=find(name);assert(i>=0);FILE*out=fopen(dest,"wb");assert(out);rewind(files[i].f);
    unsigned char buf[4096];size_t n;while((n=fread(buf,1,sizeof(buf),files[i].f)))assert(fwrite(buf,1,n,out)==n);
    fclose(out);
}
static void stop(void){fx_stop();fx_service();assert(!FX(active)&&!allocated&&!status().allocated&&!fx_filter(0));}
