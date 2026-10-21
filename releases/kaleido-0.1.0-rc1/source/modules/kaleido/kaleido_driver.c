/* SPDX-License-Identifier: GPL-2.0-or-later; Max Seelig (MAXmade). */
#include "fx_mirror.h"
#include "effect_config.h"
#include <string.h>
static int validate(const fx_params*p)
{return p&&p->value[0]<360&&(p->value[1]==4||p->value[1]==6||p->value[1]==12)&&p->value[2]<=1000&&p->value[3]<=1000;}
static int render(const fx_state*old,fx_state*next,const void*source,const void*history,void*dest,size_t cap,
    unsigned width,unsigned height,unsigned pitch,const uint32_t*times,uint32_t*new_times,
    const fx_params*p,unsigned now,void*workspace,size_t workspace_size)
{
    (void)history;(void)times;(void)new_times;
    int error=fx_mirror_render(source,dest,cap,workspace,workspace_size,p,width,height,pitch);
    if(error)return error;
    memset(next,0,sizeof(*next));next->params=*p;next->initialized=1;next->width=width;next->height=height;
    next->last_ms=now;next->frames=old->frames+1;return FX_OK;
}
const fx_effect FX(effect)={
    .module="kaleido",.title="Kaleido",.version=FX_VERSION,.prefix="KAL",.log_prefix="KAL",
    .kind="native-kaleidoscope",.description="Radial kaleidoscope in LiveView and native Canon CR2",
    .param_count=4,.param_names={"rotation_deg","segments","center_x_per_mille","center_y_per_mille"},
    .defaults={{0,6,500,500}},.workspace_size=FX_MIRROR_MAX_WORKSPACE,
    .validate=validate,.ready=fx_mirror_ready,.prepare=fx_mirror_prepare,.render=render,
};
