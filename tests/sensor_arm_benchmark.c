/* Same packed sensor operation on host and ARMv5TE; reduced sensor fixture. */
#include "fx_mirror_sensor.h"
#include "fx_copy.h"
#ifdef SENSOR_ARM
void*memcpy(void*d,const void*s,size_t n)
{unsigned char*a=d;const unsigned char*b=s;while(n--)*a++=*b++;return d;}
void*memset(void*d,int v,size_t n)
{unsigned char*a=d;while(n--)*a++=(unsigned char)v;return d;}
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
static fx_mirror_raw map;
unsigned preview_arm_test(void*source,void*out,void*work,const unsigned*config)
{
    if(!config[0]){
        fx_copy_cpu(out,source,691200);
        gl_frame f={out,560u*212u,320,212,560,GL_CANON14,0,16383};
        const uint32_t region[]={4,8,208,312};
        fx_params p={{config[1],config[2],config[3],config[4]}};
        return fx_mirror_raw_prepare(&map,&f,region,&p,0);
    }
    size_t bytes=fx_mirror_sensor_inplace_size(&map);
#ifdef SENSOR_CURRENT
    bytes=2097152u*(uint64_t)(map.cells_w*map.cells_h)/4478976u;
#endif
    return fx_mirror_sensor_inplace(&map,out,map.source.size,work,bytes);
}
#ifndef SENSOR_ARM
int main(int argc,char**argv)
{
    if(argc!=6)return 2;
    unsigned config[]={0,(unsigned)atoi(argv[2]),(unsigned)atoi(argv[3]),(unsigned)atoi(argv[4]),(unsigned)atoi(argv[5])};
    unsigned char*source=malloc(691200),*out=malloc(691200);void*work=malloc(2097152);
    if(!source||!out||!work)return 3;
    for(unsigned i=0;i<691200;i++)source[i]=(unsigned char)(i*37u+i/51u);
    if(preview_arm_test(source,out,work,config))return 4;
    config[0]=1;if(preview_arm_test(source,out,work,config))return 5;
    FILE*f=fopen(argv[1],"wb");if(!f||fwrite(out,1,691200,f)!=691200||fclose(f))return 6;
    free(work);free(out);free(source);return 0;
}
#endif
