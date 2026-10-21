/* SPDX-License-Identifier: GPL-2.0-or-later. All U/Y/V bytes vs release rc1. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fx_mirror.h"
extern size_t baseline_mirror_size(unsigned,unsigned);
extern int baseline_mirror_prepare(void*,size_t,const fx_params*,unsigned,unsigned,unsigned,volatile int*);
extern int baseline_mirror_render(const void*,void*,size_t,const void*,size_t,const fx_params*,unsigned,unsigned,unsigned);
static unsigned cases,lookups,fallbacks;
static void compare(unsigned w,unsigned h,unsigned pad,unsigned alignment,const fx_params*p)
{
    unsigned pitch=w*2+pad;size_t bytes=(size_t)pitch*h;
    size_t old_size=baseline_mirror_size(w,h),new_size=fx_mirror_size(w,h);
    unsigned char*raw=malloc(bytes+32),*source=raw+alignment;
    unsigned char*a_raw=malloc(bytes+32),*b_raw=malloc(bytes+32),*a=a_raw+alignment,*b=b_raw+alignment;
    void*old=malloc(old_size),*next=malloc(new_size);assert(raw&&a_raw&&b_raw&&old&&next);
    for(size_t i=0;i<bytes;i++)source[i]=(unsigned char)(i*37u+(i/51u)*13u);
    memset(a_raw,0xe5,bytes+32);memset(b_raw,0xe5,bytes+32);
    assert(!baseline_mirror_prepare(old,old_size,p,w,h,pitch,0));
    volatile int stop=1;assert(fx_mirror_prepare(next,new_size,p,w,h,pitch,&stop)==FX_CANCEL);stop=0;
    assert(!fx_mirror_prepare(next,new_size,p,w,h,pitch,&stop));
    unsigned mode=((fx_mirror_map*)next)->mode;lookups+=mode==3;fallbacks+=mode==2;
    assert(!baseline_mirror_render(source,a,bytes,old,old_size,p,w,h,pitch));
    assert(!fx_mirror_render(source,b,bytes,next,new_size,p,w,h,pitch));
    if(memcmp(a_raw,b_raw,bytes+32)){
        fprintf(stderr,"Mismatch %ux%u pitch=%u align=%u angle=%u count=%u center=%u,%u mode=%u\n",w,h,pitch,alignment,p->value[0],p->value[1],p->value[2],p->value[3],mode);abort();
    }
    for(unsigned i=0;i<16;i++)assert(b[bytes+i]==0xe5);
    cases++;free(raw);free(a_raw);free(b_raw);free(old);free(next);
}
int main(void)
{
    const unsigned counts[]={4,6,12},angles[]={0,1,17,45,90,179,270,315,359};
    for(unsigned c=0;c<3;c++)for(unsigned a=0;a<9;a++)for(unsigned center=0;center<3;center++){
        fx_params p={{angles[a],counts[c],center==0?0:center==1?500:1000,center==0?1000:center==1?500:0}};
        compare(720,480,0,0,&p);
        compare(32,19,0,1,&p); /* exact fallback for unaligned transport */
        compare(14,17,4,0,&p); /* padded rows and unrolled-loop tail */
        compare(10,7,1,0,&p); /* non-word pitch */
    }
    for(unsigned w=4;w<=18;w+=2)for(unsigned h=1;h<=3;h++){
        fx_params p={{17,6,425,575}};compare(w,h,0,0,&p);
    }
    fx_params legacy={{17,10,500,500}};compare(720,480,0,0,&legacy);
    assert(lookups&&fallbacks);
    printf("Full UYVY parity: %u configurations; %u lookup preparations; %u exact legacy fallbacks PASS\n",cases,lookups,fallbacks);
    return 0;
}
