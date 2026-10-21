/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "fx_mirror_sensor.h"
#include "fx_mirror.h"
#include "kaleido_settings.h"

static unsigned get14(const unsigned char*p,unsigned x)
{
    /* Independent oracle: interpret seven little-endian words as one
     * 112-bit MSB-first stream, never use the production unpacker. */
    p+=(x/8)*14;unsigned start=(x%8)*14,v=0;
    for(unsigned b=0;b<14;b++){
        unsigned bit=start+b,word=(p[(bit/16)*2]|((unsigned)p[(bit/16)*2+1]<<8));
        v=(v<<1)|((word>>(15-bit%16))&1);
    }
    return v;
}
static unsigned failures,checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL line %u: %s\n",__LINE__,#x);failures++;return;}}while(0)
static void fixture(unsigned width,unsigned height,unsigned pitch,const uint32_t region[4],const fx_params*p,int full)
{
    size_t bytes=(size_t)pitch*height;unsigned char*data=malloc(bytes),*original=malloc(bytes);
    for(size_t i=0;i<bytes;i++)data[i]=(unsigned char)((i*37u+(i/17u)*13u)^0x5au);
    memcpy(original,data,bytes);
    gl_frame source={original,bytes,width,height,pitch,GL_CANON14,0,16383};
    gl_frame target=source;target.data=data;fx_mirror_raw a,b;
    CHECK(fx_mirror_raw_prepare(&a,&source,region,p,0)==FX_OK);
    CHECK(fx_mirror_raw_prepare(&b,&target,region,p,0)==FX_OK);
    size_t needed=fx_mirror_sensor_inplace_size(&b);CHECK(needed==(b.cells_w*b.cells_h+4u)/5u);
    unsigned char*scratch=malloc(needed+32);memset(scratch,0xe5,needed+32);
    CHECK(fx_mirror_sensor_inplace(&b,data,bytes,scratch,needed-1)==FX_INVALID);
    CHECK(!memcmp(data,original,bytes));
    volatile int stop=1;b.stop=&stop;
    CHECK(fx_mirror_sensor_inplace(&b,data,bytes,scratch,needed)==FX_CANCEL);
    CHECK(!memcmp(data,original,bytes));b.stop=0;
    clock_t begin=clock();
    CHECK(fx_mirror_sensor_inplace(&b,data,bytes,scratch,needed)==FX_OK);
    double elapsed=(double)(clock()-begin)/CLOCKS_PER_SEC;
    unsigned mismatches=0,changed=0;
    for(unsigned y=0;y<height;y++){
        for(unsigned x=0;x<width;x++){
            unsigned xy[2];CHECK(fx_mirror_raw_point(&a,x,y,xy)==FX_OK);
            CHECK((xy[0]&1)==(x&1)&&(xy[1]&1)==(y&1));
            unsigned want=get14(original+(size_t)xy[1]*pitch,xy[0]);
            unsigned got=get14(data+(size_t)y*pitch,x);
            mismatches+=want!=got;changed+=got!=get14(original+(size_t)y*pitch,x);
        }
        CHECK(!memcmp(data+(size_t)y*pitch+width*14/8,original+(size_t)y*pitch+width*14/8,pitch-width*14/8));
    }
    CHECK(!mismatches);
    if(p->value[2]==500&&p->value[3]==500)CHECK(changed>0);
    for(unsigned i=0;i<32;i++)CHECK(scratch[needed+i]==0xe5);
    if(full)printf("FULL_RAW segments=%u angle=%u center=%u,%u cells=%u scratch=%zu seconds=%.3f mismatches=%u\n",
        p->value[1],p->value[0],p->value[2],p->value[3],b.cells_w*b.cells_h,needed,elapsed,mismatches);
    free(data);free(original);free(scratch);
}
static void preview(const fx_params*p)
{
    unsigned w=720,h=480,pitch=w*2;size_t bytes=(size_t)pitch*h;
    unsigned char*source=malloc(bytes),*copy=malloc(bytes),*dest=malloc(bytes+32);
    for(size_t i=0;i<bytes;i++)source[i]=(unsigned char)(i*37u+i/51);
    memcpy(copy,source,bytes);memset(dest,0xe5,bytes+32);
    size_t n=fx_mirror_size(w,h);void*work=malloc(n);volatile int stop=1;
    CHECK(fx_mirror_prepare(work,n,p,w,h,pitch,&stop)==FX_CANCEL);stop=0;
    CHECK(fx_mirror_prepare(work,n,p,w,h,pitch,&stop)==FX_OK);
    CHECK(fx_mirror_render(source,dest,bytes,work,n,p,w,h,pitch)==FX_OK);
    CHECK(!memcmp(source,copy,bytes));
    if(p->value[2]==500&&p->value[3]==500)CHECK(memcmp(source,dest,bytes));
    /* The raw mapper uses Bayer cells and its own sector implementation.
     * A 2x sensor makes one cell equal one LCD pixel for an exact Y oracle. */
    size_t raw_bytes=(size_t)w*h*8;void*raw=malloc(raw_bytes);
    gl_frame frame={raw,raw_bytes,w*2,h*2,w*4,GL_U16,0,16383};
    uint32_t region[]={0,0,h*2,w*2};fx_mirror_raw mapping;
    CHECK(fx_mirror_raw_prepare(&mapping,&frame,region,p,0)==FX_OK);
    for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
        unsigned xy[2];CHECK(fx_mirror_raw_point(&mapping,x*2,y*2,xy)==FX_OK);
        unsigned sx=xy[0]/2,sy=xy[1]/2;
        CHECK(dest[y*pitch+x*2+1]==source[sy*pitch+sx*2+1]);
    }
    CHECK(fx_mirror_render(source,source,bytes,work,n,p,w,h,pitch)==FX_INVALID);
    for(unsigned i=0;i<32;i++)CHECK(dest[bytes+i]==0xe5);
    free(raw);free(source);free(copy);free(dest);free(work);
}
static void export_geometry(void)
{
    FILE*f=fopen("evidence/geometry.json","wb");CHECK(f);
    unsigned char pixels[144*96*2];gl_frame frame={pixels,sizeof(pixels),144,96,288,GL_U16,0,16383};
    uint32_t region[]={0,0,96,144};const unsigned counts[]={4,6,12};
    fprintf(f,"[\n");unsigned first=1;
    for(unsigned c=0;c<3;c++)for(unsigned a=0;a<4;a++){
        fx_params p={{a==0?0:a==1?17:a==2?90:179,counts[c],a==2?0:500,a==3?1000:500}};fx_mirror_raw m;
        CHECK(fx_mirror_raw_prepare(&m,&frame,region,&p,0)==FX_OK);
        fprintf(f,"%s{\"segments\":%u,\"rotation\":%u,\"centerX\":%u,\"centerY\":%u,\"points\":[",first?"":",\n",p.value[1],p.value[0],p.value[2],p.value[3]);first=0;
        for(unsigned i=0;i<100;i++){
            unsigned x=(i*37)%72,y=(i*17)%48,xy[2];CHECK(fx_mirror_raw_point(&m,x*2,y*2,xy)==FX_OK);
            fprintf(f,"%s[%u,%u,%u,%u]",i?",":"",x,y,xy[0]/2,xy[1]/2);
        }
        fprintf(f,"]}");
    }
    fprintf(f,"\n]\n");fclose(f);
}
int main(int argc,char**argv)
{
    export_geometry();
    const unsigned counts[]={4,6,12},angles[]={0,17,90,179,270,359};
    uint32_t small[]={3,5,49,75};
    for(unsigned c=0;c<3;c++)for(unsigned a=0;a<6;a++)for(unsigned center=0;center<3;center++){
        fx_params p={{angles[a],counts[c],center==0?0: center==1?500:1000,center==0?1000:center==1?500:0}};
        fixture(80,52,148,small,&p,0);preview(&p);
    }
    if(argc>1&&!strcmp(argv[1],"--full")){
        uint32_t full[]={64,84,3520,5268};
        for(unsigned c=0;c<3;c++){
            fx_params p={{17,counts[c],500,500}};
            fixture(5280,3528,9240,full,&p,1);
        }
    }
    printf("Native checks: %u; failures: %u\n",checks,failures);return failures?1:0;
}
