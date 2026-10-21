/* Real metadata observer, synthetic pinned patch/metadata/SD transport only. */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "mirror_trace_sdk.h"
#include "kaleido_raw_trace.h"
#include "kaleido_capture_gate.h"
#ifndef FX_GATE_INTEGRATION
void kaleido_gate_reader(unsigned pc){(void)pc;}
#else
static patch_hook_function_cbr stage_hook;
static unsigned stage_bad,stage_fail,gate_readout,gate_overlap,gate_shutdown,gate_cancel;
static unsigned gate_rearm_io,gate_shutdown_io,stage_cancel_install,stage_shutdown_install;
#endif
int lv=1,lv_paused,gui_state,ml_shutdown_requested;
struct test_lens lens_info;
static unsigned now=1000,us=1000000,irq,in_hook,menu,half,ready=1;
static unsigned patch_calls,guard_reads,metadata_reads,fail_patch,fail_io,rearm_io,cancel_install,shutdown_install;
static unsigned next_source=0x12345678,which_desc=0x70340,task_id=0x14;
static fx_params delivered_params={{0,6,500,500}};
static uint32_t raw[42],sss[42],jpeg[42];
static const unsigned pcs[]={0xff1707e8u,0xff1686d0u,0xff429210u,0xff4292b8u,0xff168128u,0xff429280u,0xff22dac0u,0xff22da74u};
static const unsigned originals[8][4]={
    {0xe5860058u,0xe5867088u,0xe5869070u,0xe5950094u},
    {0xe5870058u,0xe587a070u,0xe587b088u,0xe5960050u},
    {0xe92d407cu,0xe51f5edcu,0xe1a04000u,0xe5950004u},
    {0xe92d4070u,0xe1a05000u,0xe51f0f88u,0xe3a04000u},
    {0xe5860058u,0xe59d0000u,0xe2800b11u,0xe2800f41u},
    {0xe92d4070u,0xe1a05000u,0xe51f0f50u,0xe3a04000u},
    {0xe5900000u,0xe5922000u,0xe2811e25u,0xe1500002u},
    {0xe92d4010u,0xe51f36f4u,0xe5902004u,0xe5900000u},
};
static unsigned bad_guard=99;
static patch_hook_function_cbr hooks[8];
static unsigned external_cache_line;
static int cache_collision(uintptr_t address)
{
    for(unsigned i=0;i<8;i++)if(hooks[i]&&
        (address&0x7e0u)==(pcs[i]&0x7e0u)&&(address&~31u)!=(pcs[i]&~31u))return 1;
    return external_cache_line&&(address&0x7e0u)==(external_cache_line&0x7e0u)&&
        (address&~31u)!=(external_cache_line&~31u);
}
static struct {char name[64];FILE*f;} files[32];static unsigned file_count;
unsigned cli(void){return irq++;}
void sei(unsigned previous){assert(irq==previous+1);irq=previous;}
unsigned get_ms_clock(void){return now;}
unsigned get_us_clock(void){us+=10;return us;}
int get_current_task_id(void){assert(in_hook&&irq);return (int)task_id;}
int gui_menu_shown(void){return (int)menu;}
int get_halfshutter_pressed(void){return (int)half;}
int fx_delivered_state(fx_state*out)
{
    unsigned s=cli();memset(out,0,sizeof(*out));out->initialized=ready;out->last_ms=now-33;
    out->params=delivered_params;sei(s);return (int)ready;
}
uint32_t read_value(uint8_t*ptr,int instruction)
{
    assert(!in_hook&&!irq&&instruction==1);uintptr_t address=(uintptr_t)ptr;
    for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++)if(address==pcs[i]+j*4){
        guard_reads++;assert(!hooks[i]);return originals[i][j]^((i*4+j)==bad_guard?1:0);
    }
#ifdef FX_GATE_INTEGRATION
    static const uint32_t stage_words[]={0xe12fff1e,0xe3a00000,0xe12fff1e,0xe12fff1e};
    for(unsigned i=0;i<4;i++)if(address==0xff3608d8u+i*4){
        if(stage_cancel_install){stage_cancel_install=0;kaleido_gate_cancel();}
        if(stage_shutdown_install){stage_shutdown_install=0;kaleido_gate_shutdown();}
        return stage_words[i]^(stage_bad?1u:0u);}
#endif
    abort();
}
static void fire(unsigned index,unsigned invalid)
{
    assert(index<8&&hooks[index]);uint32_t frame[23],before[23];
    uint32_t*regs=frame+1,*stack=regs+14;frame[0]=0xa0000013;
    for(unsigned i=0;i<14;i++)regs[i]=0x98760000+i;
    for(unsigned i=0;i<8;i++)stack[i]=0x45670000+i;
    if(index==0){regs[0]=next_source;regs[6]=0x70340;regs[5]=0x40334;}
    else if(index==1){regs[0]=next_source;regs[7]=0x6fce4;regs[6]=0x4018c;}
    else if(index==2)regs[1]=which_desc;
    else if(index==4){regs[0]=next_source;regs[6]=0x6fd8c;regs[7]=0x4018c;}
    else if(index>=6){regs[0]=0x40e00064;regs[1]=next_source;}
    else regs[0]=which_desc;
    if(invalid){if(index==0)regs[6]=0xff000000;else if(index==1)regs[7]=0xff000000;
        else if(index==2)regs[1]=0xff000000;else if(index==4)regs[6]=0xff000000;else regs[0]=0xff000000;}
    memcpy(before,frame,sizeof(frame));
    unsigned prior=in_hook;in_hook=1;hooks[index](regs,stack,pcs[index]);in_hook=prior;
    assert(!memcmp(frame,before,sizeof(frame))&&!irq);
}
int patch_hook_function(uintptr_t address,uint32_t original,patch_hook_function_cbr callback,const char*description)
{
#ifdef FX_GATE_INTEGRATION
    if(address==0xff3608d8u){assert(original==0xe12fff1e&&!stage_hook&&!in_hook&&!irq);
        assert(strstr(description,"scheduling"));if(stage_fail||cache_collision(address))return 0x10;
        stage_hook=callback;return 0;}
#endif
    assert(!irq&&!in_hook&&strstr(description,"RAW metadata"));patch_calls++;now+=2;
    unsigned i=0;while(i<8&&pcs[i]!=address)i++;assert(i<8&&original==originals[i][0]&&!hooks[i]);
    if(patch_calls==fail_patch||cache_collision(address))return 0x10;
    hooks[i]=callback;
    fire(i,0); /* Incoming call during partial installation stays passive. */
    if(cancel_install)kaleido_raw_trace_cancel();
    if(shutdown_install)kaleido_raw_trace_shutdown();
    return 0;
}
uint32_t kaleido_trace_test_read32(uintptr_t address)
{
    assert(in_hook&&irq);metadata_reads++;
    if(address>=0x70340&&address<0x703e8&&!(address&3))return raw[(address-0x70340)/4];
    if(address>=0x6fce4&&address<0x6fd8c&&!(address&3))return sss[(address-0x6fce4)/4];
    if(address>=0x6fd8c&&address<0x6fe34&&!(address&3))return jpeg[(address-0x6fd8c)/4];
    if(address==0x403b8)return 0x77;
    if(address==0x403bc)return 0x88;
    if(address==0x401c4)return 0x99;
    if(address==0x401d0)return 0xaa;
#ifdef FX_GATE_INTEGRATION
    if(address==0x3f488)return 0x40e00064;
#endif
    abort(); /* Never permit a sensor/output/argument pointer to be read. */
}
unsigned edmac_get_state(unsigned channel){assert(in_hook&&irq&&(channel==0||channel==10));return 0;}
unsigned edmac_get_flags(unsigned channel){assert(in_hook&&irq&&(channel==0||channel==10));
#ifdef FX_GATE_INTEGRATION
    if(gate_readout==1&&channel==0)return 0;
#endif
    return 0x20000000;}
uintptr_t edmac_get_address(unsigned channel){assert(in_hook&&irq&&(channel==0||channel==10));
#ifdef FX_GATE_INTEGRATION
    if(gate_readout==1&&channel==0)return 0x080090c4;
#endif
    return channel==10?0x08004894:0x04000000;}
uintptr_t edmac_get_pointer(unsigned channel){assert(in_hook&&irq&&(channel==0||channel==10));
#ifdef FX_GATE_INTEGRATION
    if(gate_readout==1&&channel==0)return 0x09f21f9c;
#endif
    return 0x05f16a80;}
struct edmac_info edmac_get_info(unsigned channel)
{assert(in_hook&&irq&&(channel==0||channel==10));struct edmac_info info={0};
#ifdef FX_GATE_INTEGRATION
    if(gate_readout==1&&channel==0){info.xa=info.xb=9240;info.xn=3528;return info;}
#endif
    info.xb=9240;info.yb=3527;return info;}
int is_dir(const char*p){assert(!in_hook&&!irq&&!strcmp(p,"ML/LOGS"));return 1;}
int FIO_CreateDirectory(const char*p){(void)p;abort();}
int FIO_GetFileSize(const char*p,uint32_t*size)
{
    assert(!in_hook&&!irq);for(unsigned i=0;i<file_count;i++)if(!strcmp(files[i].name,p)){*size=1;return 0;}return -1;
}
FILE*FIO_CreateFile(const char*p)
{
    assert(!in_hook&&!irq&&!lens_info.job_state&&file_count<32&&
        (!strncmp(p,"ML/LOGS/MTR",11)||!strncmp(p,"ML/LOGS/MCG",11)||!strncmp(p,"ML/LOGS/MSR",11)||!strncmp(p,"ML/LOGS/MPT",11)));
    strcpy(files[file_count].name,p);files[file_count].f=tmpfile();assert(files[file_count].f);return files[file_count++].f;
}
int FIO_WriteFile(FILE*f,const void*p,size_t n)
{
    assert(!in_hook&&!irq&&!lens_info.job_state);
#ifdef FX_GATE_INTEGRATION
    if(gate_rearm_io){gate_rearm_io=0;assert(!kaleido_gate_request());}
    if(gate_shutdown_io){gate_shutdown_io=0;kaleido_gate_shutdown();}
#endif
    if(rearm_io){rearm_io=0;assert(!kaleido_raw_trace_request());}
    return fail_io?0:(int)fwrite(p,1,n,f);
}
void FIO_CloseFile(FILE*f){assert(!in_hook&&!irq);fflush(f);}
static kaleido_raw_trace_report status(void){kaleido_raw_trace_report r;kaleido_raw_trace_status(&r);return r;}
static void seed(void)
{
    raw[0x38/4]=2640u<<16;raw[0x3c/4]=3528;raw[0x40/4]=0;
    raw[0x48/4]=8;raw[0x50/4]=0x40001000;raw[0x58/4]=0xdeadbeef;raw[0x78/4]=1;
    raw[0x88/4]=0xffee1234;raw[0xa0/4]=0xffffabcd;memcpy(sss,raw,sizeof(raw));memcpy(jpeg,raw,sizeof(raw));
}
static void arm(void)
{
    assert(kaleido_raw_trace_request());kaleido_raw_trace_service();
    assert(status().phase==MT_ARMED&&status().installed==255&&guard_reads==32&&patch_calls==8&&!metadata_reads);
}
int main(int argc,char**argv)
{
    assert(argc==2);seed();
    if(!strcmp(argv[1],"guard")){
        for(unsigned i=0;i<32;i++){
            bad_guard=i;assert(kaleido_raw_trace_request());kaleido_raw_trace_service();
            assert(status().phase==MT_FAILED&&status().error==MT_GUARD&&status().error_pc==pcs[i/4]+(i%4)*4);
            assert(!patch_calls&&!status().installed&&status().log[0]);
        }
    }else if(!strcmp(argv[1],"partial")){
        fail_patch=2;assert(kaleido_raw_trace_request());kaleido_raw_trace_service();
        assert(status().error==MT_PATCH&&status().core_error==0x10&&status().installed==1&&!metadata_reads);
        fire(0,0);assert(!metadata_reads);fail_patch=0;assert(kaleido_raw_trace_request());kaleido_raw_trace_service();
        assert(status().phase==MT_ARMED&&status().installed==255&&patch_calls==9&&guard_reads==60);
    }else if(!strcmp(argv[1],"cache-collision")){
        external_cache_line=0xff8007e0;
        assert(kaleido_raw_trace_request());kaleido_raw_trace_service();
        assert(status().error==MT_PATCH&&status().core_error==0x10&&status().installed==0);
        assert(status().error_pc==pcs[0]&&patch_calls==1&&!metadata_reads);
        assert(external_cache_line==0xff8007e0); /* Never evict another patch. */
    }else if(!strcmp(argv[1],"shutdown")){
        shutdown_install=1;assert(kaleido_raw_trace_request());kaleido_raw_trace_service();
        assert(status().error==MT_SHUTDOWN&&status().installed==1&&patch_calls==1&&!file_count);
        fire(0,0);assert(!metadata_reads&&!kaleido_raw_trace_request());
    }else if(!strcmp(argv[1],"cancel")){
        cancel_install=1;assert(kaleido_raw_trace_request());kaleido_raw_trace_service();
        assert(status().error==MT_CANCEL&&status().installed==1&&patch_calls==1&&status().log[0]);
        fire(0,0);assert(!metadata_reads);
    }else if(!strcmp(argv[1],"timeout")){
        now=0xfffffff0u;arm();now+=60001;kaleido_raw_trace_service();assert(status().error==MT_TIMEOUT&&status().log[0]);
    }else if(!strcmp(argv[1],"no-preview")){
        ready=0;assert(kaleido_raw_trace_request());kaleido_raw_trace_service();
        assert(status().error==MT_NO_PREVIEW&&!patch_calls&&status().log[0]);
    }else if(!strcmp(argv[1],"log-failure")){
        arm();fire(3,0);now+=800;fail_io=1;kaleido_raw_trace_service();assert(status().error==MT_LOG&&!status().log[0]);
    }else if(!strcmp(argv[1],"ring")){
        arm();for(unsigned i=0;i<72;i++){now++;fire(i&1?2:3,0);}now+=800;kaleido_raw_trace_service();
        assert(status().phase==MT_DONE&&status().events==64&&status().dropped==8&&status().starts==36&&status().log[0]);
    }else if(!strcmp(argv[1],"accessors")){
        arm();unsigned reads=metadata_reads;
        assert(cache_collision(0xff22dab4)); /* Actual dev11/TTL_Start collision. */
        assert(!cache_collision(0xff22dac0));
        uint32_t frame[23]={0},before[23]={0};
        hooks[6](frame+1,frame+14,pcs[6]); /* wrong pinned stack relation */
        hooks[6](0,0,pcs[6]);
        assert(!status().events&&!memcmp(frame,before,sizeof(frame)));
        fire(7,0);fire(6,0);
        assert(metadata_reads==reads); /* register-only; even job pointers cannot be read */
        which_desc=0x6fd8c;jpeg[0x48/4]=10;fire(4,0);fire(5,0);fire(3,0);
        now+=800;kaleido_raw_trace_service();assert(status().phase==MT_DONE&&status().events==5);
        char log[16384]={0};rewind(files[0].f);fread(log,1,sizeof(log)-1,files[0].f);
        assert(strstr(log,"original_cpsr=a0000013 argument_job=40e00064 argument_source=12345678 reader_valid=0"));
        assert(strstr(log,"reader_valid=1 reader_before=0 reader_after=0 reader_address=8004894"));
    }else{
        assert(!strcmp(argv[1],"normal"));menu=1;assert(kaleido_raw_trace_request());kaleido_raw_trace_service();assert(!patch_calls);
        menu=0;half=1;kaleido_raw_trace_service();assert(!patch_calls);half=0;lv=0;kaleido_raw_trace_service();assert(!patch_calls);
        lv=1;gui_state=3;kaleido_raw_trace_service();assert(!patch_calls);gui_state=0;lens_info.job_state=2;kaleido_raw_trace_service();assert(!patch_calls);
        lens_info.job_state=0;kaleido_raw_trace_service();assert(status().phase==MT_ARMED&&status().installed==255&&guard_reads==32&&!metadata_reads);
        for(unsigned i=0;i<6;i++)fire(i,1);
        assert(status().skipped==6&&!metadata_reads);
        now++;fire(0,0);raw[0x58/4]=next_source;now++;fire(2,0);now++;fire(3,0);
        which_desc=0x6fce4;next_source=0x06440000;now++;fire(1,0);sss[0x58/4]=next_source;now++;fire(2,0);now++;fire(3,0);
        which_desc=0x6fd8c;next_source=0x05440000;now++;fire(4,0);jpeg[0x58/4]=next_source;now++;fire(5,0);now++;fire(3,0);
        assert(status().events==9&&status().starts==3&&status().max_hook_us==10);
        now+=749;kaleido_raw_trace_service();assert(status().phase==MT_ARMED&&!file_count);
        now++;lens_info.job_state=1;kaleido_raw_trace_service();assert(status().phase==MT_ARMED&&!file_count);
        lens_info.job_state=0;rearm_io=1;kaleido_raw_trace_service();assert(status().phase==MT_DONE&&status().log[0]&&file_count==1);
        char log[16384]={0};rewind(files[0].f);fread(log,1,sizeof(log)-1,files[0].f);
        assert(strstr(log,"source_field=deadbeef effective_source=12345678")&&strstr(log,"desc=6fce4")&&strstr(log,"params=90,2,500,500"));
        assert(strstr(log,"xb=9240 yb=3527")&&strstr(log,"No owned RAW lease")&&strstr(log,"events=9"));
        unsigned reads=metadata_reads;fire(0,0);assert(metadata_reads==reads);
        assert(kaleido_raw_trace_request());kaleido_raw_trace_service();assert(patch_calls==8&&status().phase==MT_ARMED); /* Reuse resident hooks. */
        kaleido_raw_trace_cancel();kaleido_raw_trace_service();assert(status().error==MT_CANCEL&&file_count==2);
        kaleido_raw_trace_shutdown();fire(0,0);assert(metadata_reads==reads&&!kaleido_raw_trace_request());
    }
    if(file_count){
        /* Export actual logger bytes for the independent read-only analyzer.
         * All records are generated by synthetic SDK transport, not a camera. */
        char path[128];snprintf(path,sizeof(path),"build/mirror-trace-fixtures/%s.LOG",argv[1]);
        FILE*out=fopen(path,"wb");assert(out);rewind(files[0].f);int ch;
        while((ch=fgetc(files[0].f))!=EOF)assert(fputc(ch,out)!=EOF);
        assert(!fclose(out));
    }
    for(unsigned i=0;i<file_count;i++)fclose(files[i].f);
    assert(!irq);printf("Native CR2 metadata trace %s: PASS (synthetic patch/metadata/SD transport; no camera execution)\n",argv[1]);return 0;
}
