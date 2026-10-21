/* Actual camera lifecycle and Kaleido renderer, synthetic Canon SDK only. */
#include "fx_camera_fake.h"
#include "fx_mirror.h"
#include "fx_mirror_sensor.h"
static void reject_frame(void){half=1;fx_half(1);}
static void check_fill(const void*memory,size_t size,unsigned char value)
{const unsigned char*p=memory;for(size_t i=0;i<size;i++)assert(p[i]==value);}
int main(void)
{
    (void)dump;
    const unsigned minimum=(2592u*1728u+4u)/5u;
    fx_camera_init();assert(fx_native_capture_mode());assert(fx_capture_scratch_prefer(minimum,2*1024*1024));
    assert(fx_capture_scratch_share_workspace());
    assert(!fx_capture_scratch_prefer(1,2*1024*1024));
    unsigned initial=free_bytes;assert(fx_start());fx_service();
    assert(status().running&&status().required==2*1024*1024+1536*1024&&allocation_count==1);
    assert(!fx_capture_scratch_share_workspace());
    frame(30);fx_state delivered;assert(fx_delivered_state(&delivered));
    fx_capture_scratch lease;assert(fx_capture_scratch_acquire(&lease)&&lease.size==2*1024*1024);
    /* Actual packed sensor backend borrows and destroys the preview map. */
    unsigned char sensor[80*52*14/8],original[sizeof(sensor)];
    for(unsigned i=0;i<sizeof(sensor);i++)sensor[i]=(unsigned char)(i*37+i/51);
    memcpy(original,sensor,sizeof(sensor));
    gl_frame raw={sensor,sizeof(sensor),80,52,140,GL_CANON14,0,16383};
    const uint32_t region[]={0,0,52,80};fx_mirror_raw mapping;
    assert(!fx_mirror_raw_prepare(&mapping,&raw,region,&delivered.params,0));
    assert(!fx_mirror_sensor_inplace(&mapping,sensor,sizeof(sensor),lease.data,lease.size));
    assert(memcmp(original,sensor,sizeof(sensor)));
    memset(lease.data,0xab,lease.size);assert(fx_capture_scratch_release(&lease));
    assert(!fx_delivered_state(&delivered));
    /* A stale filter after release must not interpret RAW bitmap bytes as a
     * prepared lookup. Service must rebuild the overwritten map first. */
    frame(40);assert(!fx_delivered_state(&delivered));
    fx_service();frame(40);assert(fx_delivered_state(&delivered));
    unsigned frames=status().frames;frame(41);assert(status().frames==frames+1);
    assert(fx_capture_scratch_acquire(&lease));
    check_fill((unsigned char*)lease.data+FX_MIRROR_MAX_WORKSPACE,lease.size-FX_MIRROR_MAX_WORKSPACE,0xab);
    assert(fx_capture_scratch_release(&lease));fx_service();frame(50);
    fx_params changed={{17,12,425,575}};
    half=1;fx_half(1);assert(fx_controls(&changed));fx_service();frame(60);
    assert(fx_delivered_state(&delivered)&&delivered.params.value[0]==0&&status().frozen);
    half=0;fx_half(0);fx_service();frame(70);
    assert(fx_delivered_state(&delivered)&&delivered.params.value[0]==17);
    /* A half edge during a render rejects incompatible controls; fallback
     * must use Canon source, never the poison-filled RAW scratch arena. */
    changed.value[0]=33;assert(fx_controls(&changed));fx_service();
    during_filter_sync=reject_frame;frame(80);assert(!memcmp(output,source,sizeof(output)));
    assert(fx_delivered_state(&delivered)&&delivered.params.value[0]==17);
    half=0;fx_half(0);fx_service();frame(90);
    assert(fx_capture_scratch_acquire(&lease));memset(lease.data,0xcd,lease.size);
    fx_service();check_fill(lease.data,lease.size,0xcd); /* Worker cannot prepare during a lease. */
    struct display_filter_buffers b={source,output};memset(output,0xe5,sizeof(output));
    in_filter=1;assert(!fx_filter(&b));in_filter=0;check_fill(output,sizeof(output),0xe5);
    fx_stop();fx_service();assert(FX(active)&&allocated);assert(fx_capture_scratch_release(&lease));
    fx_service();assert(!FX(active)&&!allocated&&free_bytes==initial);
    assert(fx_start());fx_service();frame(35);fx_canon_gui(1);
    memset(output,0xe5,sizeof(output));in_filter=1;assert(!fx_filter(&b));in_filter=0;
    check_fill(output,sizeof(output),0xe5);
    assert(fx_key(MODULE_KEY_PLAY)==1&&fx_key(MODULE_KEY_PRESS_SET)==1&&!fx_capture());
    fx_canon_gui(0);frame(40);stop();assert(!file_count);
    /* A single arena fits while rc3's extra lookup allocation does not. */
    free_bytes=minimum+1536*1024+4096;
    fx_params four={{0,4,500,500}};assert(fx_controls(&four));
    assert(fx_start());fx_service();
    fprintf(stderr,"Four-segment low-memory startup: running=%u error=%u required=%u free_initial=%u\n",
        status().running,status().error,status().required,status().free_initial);
    assert(status().running);frame(40);stop();
    free_bytes=minimum+1536*1024+4096;
    assert(fx_start());fx_service();assert(status().running&&status().required==minimum+1536*1024);
    frame(40);assert(fx_capture_scratch_acquire(&lease)&&lease.size==minimum);
    assert(fx_capture_scratch_release(&lease));stop();
    /* Every segment/angle change must reuse the same allocation, including
     * when no further allocation would succeed. */
    free_bytes=minimum+1536*1024+4096;assert(fx_start());fx_service();
    unsigned allocations=allocation_count;
    fail_allocation=allocations+1;
    const unsigned segments[]={6,4,12,4,6,12};
    for(unsigned i=0;i<sizeof(segments)/sizeof(segments[0]);i++){
        fx_params p={{i*17,segments[i],500,500}};assert(fx_controls(&p));
        fx_service();frame(42);assert(fx_delivered_state(&delivered));
        assert(delivered.params.value[1]==segments[i]&&status().running&&!status().error);
        assert(allocation_count==allocations);
    }
    fail_allocation=0;stop();
    free_bytes=initial;fail_allocation=allocation_count+1;
    assert(fx_start());fx_service();assert(status().running);frame(40);
    assert(fx_capture_scratch_acquire(&lease)&&lease.size==minimum);
    assert(fx_capture_scratch_release(&lease));stop();
    free_bytes=minimum+1536*1024-1;
    assert(fx_start());fx_service();assert(!FX(active)&&!allocated&&status().error==10);
    /* Other native effects retain separate scratch/workspace unless opted in. */
    free_bytes=initial;fx_camera_init();assert(fx_native_capture_mode());
    assert(fx_capture_scratch_prefer(minimum,2*1024*1024));
    allocations=allocation_count;assert(fx_start());fx_service();
    assert(status().running&&allocation_count==allocations+2);
    assert(status().required==2*1024*1024+FX_MIRROR_MAX_WORKSPACE+1536*1024);
    frame(43);assert(fx_capture_scratch_acquire(&lease));memset(lease.data,0xab,lease.size);
    assert(fx_capture_scratch_release(&lease));frame(44);assert(fx_delivered_state(&delivered));
    stop();assert(free_bytes==initial);
    puts("Native camera: shared arena, 6/4/12 changes without allocations, map rebuild after RAW, half lock, scratch exclusion, stop, reserve and Canon review PASS");return 0;
}
