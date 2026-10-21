/* Whole sensor transform, deterministic packed data, no Canon/file overhead. */
#include "fx_mirror_sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static unsigned saved_cells,fallback;
void FX(sensor_profile)(unsigned cells,unsigned graph){saved_cells=cells;fallback=graph;}
int main(int argc,char**argv)
{
    if(argc!=6&&argc!=7)return 2;
    unsigned width=5280,height=3528,pitch=9240;
    size_t bytes=(size_t)pitch*height;
    unsigned char*data=malloc(bytes);if(!data)return 3;
    for(size_t i=0;i<bytes;i++)data[i]=(unsigned char)((i*37u+(i/17u)*13u)^0x5au);
    gl_frame frame={data,bytes,width,height,pitch,GL_CANON14,0,16383};
    uint32_t region[]={64,84,3520,5268};
    fx_params p={{(unsigned)atoi(argv[2]),(unsigned)atoi(argv[1]),(unsigned)atoi(argv[3]),(unsigned)atoi(argv[4])}};
    fx_mirror_raw map;
    if(fx_mirror_raw_prepare(&map,&frame,region,&p,0))return 4;
    size_t size=argc==7?(size_t)strtoul(argv[6],0,10):fx_mirror_sensor_inplace_size(&map);
    void*work=malloc(size);if(!work)return 5;
    clock_t start=clock();
    int result=fx_mirror_sensor_inplace(&map,data,bytes,work,size);
    double ms=1000.0*(clock()-start)/CLOCKS_PER_SEC;
    FILE*f=fopen(argv[5],"wb");if(!f||fwrite(data,1,bytes,f)!=bytes||fclose(f))return 6;
    printf("{\"ms\":%.3f,\"saved_cells\":%u,\"graph_fallback\":%u,\"scratch_bytes\":%zu,\"result\":%d}\n",ms,saved_cells,fallback,size,result);
    free(work);free(data);return result;
}
