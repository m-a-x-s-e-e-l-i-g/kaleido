/* SPDX-License-Identifier: GPL-2.0-or-later; Max Seelig (MAXmade). */
#include <string.h>
#ifdef FX_SETTINGS_HOST_TEST
#include "kaleido_settings_sdk.h"
#else
#include <module.h>
#include <dryos.h>
#include <config.h>
#endif
#include "kaleido_settings.h"
#include "gl_format.h"
static unsigned generation,slot=1;
static uint32_t hash(const unsigned char*p)
{uint32_t h=2166136261u;for(unsigned i=0;i<32;i++)h=(h^p[i])*16777619u;return h;}
static unsigned word(const unsigned char*p)
{return p[0]|((unsigned)p[1]<<8)|((unsigned)p[2]<<16)|((unsigned)p[3]<<24);}
static void put(unsigned char*p,unsigned v)
{for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(v>>(i*8));}
static int decode(const unsigned char*data,kaleido_settings*out,unsigned*seq)
{
    if(word(data)!=0x314c414bu||word(data+4)!=1||word(data+32)!=hash(data))return 0;
    kaleido_settings s={word(data+12),word(data+16),word(data+20),word(data+24),word(data+28)};
    if(!kaleido_settings_valid(&s))return 0;
    *out=s;*seq=word(data+8);return 1;
}
static void filename(char path[96],unsigned which)
{gl_snprintf(path,96,"%skaleido%u.prf",get_config_dir(),which);}
static int read_slot(unsigned which,kaleido_settings*out,unsigned*seq)
{
    char path[96];unsigned char data[36];uint32_t size;filename(path,which);
    if(FIO_GetFileSize(path,&size)||size!=sizeof(data))return 0;
    FILE*f=FIO_OpenFile(path,O_RDONLY|O_SYNC);if(!f)return 0;
    int count=FIO_ReadFile(f,data,sizeof(data));FIO_CloseFile(f);
    return count==(int)sizeof(data)&&decode(data,out,seq);
}
int kaleido_settings_load(kaleido_settings*out)
{
    if(!out)return 0;
    kaleido_settings a,b;unsigned ga=0,gb=0;int va=read_slot(0,&a,&ga),vb=read_slot(1,&b,&gb);
    generation=0;slot=1;
    if(!va&&!vb)return 0;
    if(vb&&(!va||(int32_t)(gb-ga)>0)){*out=b;generation=gb;slot=1;}
    else{*out=a;generation=ga;slot=0;}
    return 1;
}
int kaleido_settings_save(const kaleido_settings*s)
{
    if(!s||!kaleido_settings_valid(s))return 0;
    unsigned next=generation+1,target=slot^1u;unsigned char data[36];char path[96];
    put(data,0x314c414bu);put(data+4,1);put(data+8,next);put(data+12,s->enabled);
    put(data+16,s->segments);put(data+20,s->rotation);put(data+24,s->center_x);put(data+28,s->center_y);put(data+32,hash(data));
    filename(path,target);FILE*f=FIO_CreateFile(path);if(!f)return 0;
    int count=FIO_WriteFile(f,data,sizeof(data));FIO_CloseFile(f);
    kaleido_settings check;unsigned seq;
    if(count!=(int)sizeof(data)||!read_slot(target,&check,&seq)||seq!=next||memcmp(s,&check,sizeof(check)))return 0;
    generation=next;slot=target;return 1;
}
