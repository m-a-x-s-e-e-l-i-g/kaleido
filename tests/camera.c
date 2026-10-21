/* Actual camera lifecycle and Kaleido renderer, synthetic Canon SDK only. */
#include "fx_camera_fake.h"
#include "fx_mirror.h"
static void reject_frame(void){half=1;fx_half(1);}
static void check_fill(const void*memory,size_t size,unsigned char value)
{const unsigned char*p=memory;for(size_t i=0;i<size;i++)assert(p[i]==value);}
int main(void)
{
    (void)dump;
    const unsigned minimum=(2592u*1728u+4u)/5u;
    fx_camera_init();assert(fx_native_capture_mode());assert(fx_capture_scratch_prefer(minimum,2*1024*1024));
    unsigned initial=free_bytes;assert(fx_start());fx_service();
    assert(status().running&&status().required==2*1024*1024+FX_MIRROR_MAX_WORKSPACE+1536*1024);
    frame(30);fx_state delivered;assert(fx_delivered_state(&delivered));
    fx_capture_scratch lease;assert(fx_capture_scratch_acquire(&lease)&&lease.size==2*1024*1024);
    memset(lease.data,0xab,lease.size);assert(fx_capture_scratch_release(&lease));
    assert(!fx_delivered_state(&delivered));frame(40);assert(fx_delivered_state(&delivered));
    assert(fx_capture_scratch_acquire(&lease));check_fill(lease.data,lease.size,0xab);
    assert(fx_capture_scratch_release(&lease));frame(50);
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
    struct display_filter_buffers b={source,output};memset(output,0xe5,sizeof(output));
    in_filter=1;assert(!fx_filter(&b));in_filter=0;check_fill(output,sizeof(output),0xe5);
    fx_stop();fx_service();assert(FX(active)&&allocated);assert(fx_capture_scratch_release(&lease));
    fx_service();assert(!FX(active)&&!allocated&&free_bytes==initial);
    assert(fx_start());fx_service();frame(35);fx_canon_gui(1);
    memset(output,0xe5,sizeof(output));in_filter=1;assert(!fx_filter(&b));in_filter=0;
    check_fill(output,sizeof(output),0xe5);
    assert(fx_key(MODULE_KEY_PLAY)==1&&fx_key(MODULE_KEY_PRESS_SET)==1&&!fx_capture());
    fx_canon_gui(0);frame(40);stop();assert(!file_count);
    free_bytes=minimum+FX_MIRROR_MAX_WORKSPACE+1536*1024;
    assert(fx_start());fx_service();assert(status().running&&status().required==free_bytes+allocated);
    frame(40);assert(fx_capture_scratch_acquire(&lease)&&lease.size==minimum);
    assert(fx_capture_scratch_release(&lease));stop();
    free_bytes=initial;fail_allocation=allocation_count+1;
    assert(fx_start());fx_service();assert(status().running);frame(40);
    assert(fx_capture_scratch_acquire(&lease)&&lease.size==minimum);
    assert(fx_capture_scratch_release(&lease));stop();
    free_bytes=minimum+FX_MIRROR_MAX_WORKSPACE+1536*1024-1;
    assert(fx_start());fx_service();assert(!FX(active)&&!allocated&&status().error==10);
    puts("Native camera: no per-frame scratch copy, half lock, rejected-frame fallback, scratch exclusion, stop, reserve and Canon review PASS");return 0;
}
