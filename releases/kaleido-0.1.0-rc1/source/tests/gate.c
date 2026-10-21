/* Real handoff, safety observers, Bloom backend and sensor pixels; fake Canon. */
#define FX_GATE_INTEGRATION 1
#define main legacy_trace_main
#include "trace_transport.c"
#undef main
#include "fx_mirror_raw.h"
static unsigned char*sensor_image;
static unsigned char scratch[(2592*1728+4)/5] __attribute__((aligned(8)));
static unsigned scratch_busy,fail_scratch,sample_reads,plan_cancel,plan_calls,sensor_writes;
void FX(sensor_plan_hook)(void)
{if(plan_cancel&&++plan_calls==10)kaleido_gate_cancel();}
void FX(sensor_write_hook)(void){sensor_writes++;}
static unsigned char*original;
void*kaleido_trace_test_sensor_pointer(unsigned a){assert(a==0x48004894u);return sensor_image;}
uint16_t kaleido_trace_test_read16(uintptr_t a)
{
    assert(in_hook&&!irq&&a>=0x48004894u&&a+2<=0x48004894u+32598720u);
    sample_reads++;unsigned n=(unsigned)(a-0x48004894u);
    return (uint16_t)(sensor_image[n]|((unsigned)sensor_image[n+1]<<8));
}
void msleep(unsigned n){assert(in_hook&&!irq&&n==5);now+=n;us+=n*1000;if(gate_cancel)kaleido_gate_cancel();if(gate_shutdown)kaleido_gate_shutdown();if(gate_overlap){gate_overlap=0;fire(3,0);}}
int fx_capture_scratch_acquire(fx_capture_scratch*out)
{assert(!scratch_busy&&!irq&&in_hook);if(fail_scratch)return 0;scratch_busy=1;*out=(fx_capture_scratch){scratch,sizeof(scratch),1};return 1;}
int fx_capture_scratch_release(const fx_capture_scratch*lease)
{assert(scratch_busy&&lease->data==scratch&&lease->size==sizeof(scratch)&&lease->generation==1);scratch_busy=0;return 1;}
static void set_pixel(unsigned char*p,unsigned x,unsigned v)
{
    for(unsigned b=0;b<14;b++){
        unsigned s=(x%8)*14+b,bit=15-s%16;unsigned char*q=p+x/8*14+s/16*2+bit/8;
        unsigned mask=1u<<(bit%8);*q=(unsigned char)((*q&~mask)|((v>>(13-b)&1)?mask:0));
    }
}
static kaleido_gate_report report(void){kaleido_gate_report r;kaleido_gate_status(&r);return r;}
static void stage(unsigned bad)
{
    uint32_t frame[23]={0},before[23];uint32_t*r=frame+1;
    frame[0]=0x13;r[0]=0x48000064;r[10]=0x3f3d4;r[13]=bad?0xff15223c:0xff152238;
    memcpy(before,frame,sizeof(frame));in_hook=1;stage_hook(r,r+14,0xff3608d8);in_hook=0;
    assert(!memcmp(frame,before,sizeof(frame))&&!irq&&!scratch_busy);
}
int main(int argc,char**argv)
{
    assert(argc==2);seed();sensor_image=malloc(32598720);original=malloc(32598720);assert(sensor_image&&original);
    unsigned bad=!strcmp(argv[1],"caller"),fault=strcmp(argv[1],"repeat")!=0;
    fail_scratch=!strcmp(argv[1],"scratch");gate_cancel=!strcmp(argv[1],"cancel");
    plan_cancel=!strcmp(argv[1],"planning-cancel");
    gate_shutdown=!strcmp(argv[1],"shutdown");gate_overlap=!strcmp(argv[1],"overlap");
    kaleido_gate_automatic(1);kaleido_raw_trace_service();kaleido_gate_service();assert(report().phase==MG_ARMED);
    for(unsigned shot=0;shot<(fault?1u:3u);shot++){
        for(unsigned i=0;i<32598720;i++)sensor_image[i]=(unsigned char)(i*197+(i>>7)*17);
        unsigned x=500+shot*1000,y=1000+shot*100;
        set_pixel(sensor_image+y*9240,x,16383);memcpy(original,sensor_image,32598720);
        delivered_params=(fx_params){{17+shot*7,shot==0?4:shot==1?6:12,500,500}};
        kaleido_gate_full_press();fx_params captured=delivered_params;delivered_params.value[0]=8;
        lv=0;lens_info.job_state=20;gate_readout=1;stage(bad);
        kaleido_gate_report r=report();assert(!memcmp(&r.params,&captured,sizeof(captured))&&!file_count);
        if(fault){
            assert(r.phase==MG_FAILED&&!r.patch_done&&!sensor_writes&&!memcmp(original,sensor_image,32598720));
            assert(r.patch_started==plan_cancel);
            assert(r.error==(bad?MG_CONTEXT:fail_scratch?MG_PATCH_PREP:gate_cancel||plan_cancel?MG_CANCEL:gate_shutdown?MG_SHUTDOWN:MG_OVERLAP));
        }else{
            fprintf(stderr,"shot=%u phase=%u error=%u started=%u done=%u changed=%u mismatches=%u held=%u\n",shot,r.phase,r.error,r.patch_started,r.patch_done,r.patch_changed,r.patch_mismatches,r.held_us);
            assert(r.phase==MG_DONE&&r.patch_started&&r.patch_done&&r.patch_changed>0&&!r.error);
            assert(memcmp(original,sensor_image,32598720));
            for(unsigned row=0;row<3528;row++){
                if(row<64||row>=3520)assert(!memcmp(original+row*9240,sensor_image+row*9240,9240));
                else{assert(!memcmp(original+row*9240,sensor_image+row*9240,146));assert(!memcmp(original+row*9240+9220,sensor_image+row*9240+9220,20));}
            }
            lv=1;lens_info.job_state=0;half=1;kaleido_gate_service();assert(report().phase==MG_DONE);
            half=0;gui_state=1;kaleido_gate_service();assert(report().phase==MG_DONE);
            gui_state=0;kaleido_gate_service();assert(report().phase==MG_ARMED);
        }
    }
    kaleido_gate_shutdown();kaleido_raw_trace_shutdown();free(original);free(sensor_image);
    printf("Kaleido capture %s: PASS (actual sensor backend/handoff, synthetic transport)\n",argv[1]);return 0;
}
