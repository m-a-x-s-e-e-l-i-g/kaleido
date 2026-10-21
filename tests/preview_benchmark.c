/* SPDX-License-Identifier: GPL-2.0-or-later. Same component on host and ARM. */
#include "fx_mirror.h"
#ifdef PREVIEW_ARM
void*memcpy(void*d,const void*s,size_t n)
{unsigned char*a=d;const unsigned char*b=s;while(n--)*a++=*b++;return d;}
void*memset(void*d,int v,size_t n)
{unsigned char*a=d;while(n--)*a++=(unsigned char)v;return d;}
int memcmp(const void*a,const void*b,size_t n)
{const unsigned char*x=a,*y=b;while(n--){if(*x!=*y)return *x<*y?-1:1;x++;y++;}return 0;}
unsigned preview_arm_test(void*source,void*out,void*work,const unsigned*config)
{
    fx_params p={{config[1],config[2],config[3],config[4]}};
    size_t bytes=fx_mirror_size(720,480);
    if(config[0]==2)return (unsigned)bytes;
    if(!config[0])return fx_mirror_prepare(work,bytes,&p,720,480,1440,0);
    return fx_mirror_render(source,out,691200,work,bytes,&p,720,480,1440);
}
#else
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main(int argc,char**argv)
{
    if(argc!=6)return 2;
    fx_params p={{(unsigned)atoi(argv[2]),(unsigned)atoi(argv[3]),(unsigned)atoi(argv[4]),(unsigned)atoi(argv[5])}};
    size_t size=fx_mirror_size(720,480);void*work=malloc(size);
    unsigned char*src=malloc(691200),*out=malloc(691200);
    if(!work||!src||!out)return 3;
    for(unsigned i=0;i<691200;i++)src[i]=(unsigned char)(i*37u+i/51u);
    clock_t before=clock();
    if(fx_mirror_prepare(work,size,&p,720,480,1440,0))return 4;
    printf("{\"workspace\":%zu,\"mode\":%u,\"prepare_host_ms\":%.6f,\"samples_host_ms\":[",size,((fx_mirror_map*)work)->mode,1000.0*(clock()-before)/CLOCKS_PER_SEC);
    for(unsigned sample=0;sample<11;sample++){
        before=clock();
        for(unsigned frame=0;frame<10;frame++)if(fx_mirror_render(src,out,691200,work,size,&p,720,480,1440))return 5;
        printf("%s%.6f",sample?",":"",100.0*(clock()-before)/CLOCKS_PER_SEC);
    }
    puts("]}");FILE*f=fopen(argv[1],"wb");if(!f)return 6;
    if(fwrite(out,1,691200,f)!=691200)return 7;
    fclose(f);free(src);free(out);free(work);return 0;
}
#endif
