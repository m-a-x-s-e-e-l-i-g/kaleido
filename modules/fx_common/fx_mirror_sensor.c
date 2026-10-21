/* SPDX-License-Identifier: GPL-2.0-or-later; Max Seelig (MAXmade).
 * Bayer mapping: this project. Canon14 word layout: pinned ML raw.h / core.
 * No Canon API calls, hooks, capture requests, allocation or file formats. */
#include "fx_mirror_sensor.h"
#include "fx_mirror_edges.h"
#include "fx_mirror_trits.h"
#include "fx_copy.h"
#include <string.h>
#ifdef FX_SENSOR_PROFILE
extern void FX(sensor_profile)(unsigned saved_cells,unsigned fallback);
#endif
#ifdef FX_SENSOR_TEST_HOOK
extern void FX(sensor_write_hook)(void);
extern void FX(sensor_plan_hook)(void);
#endif
static uintptr_t address(const void*p)
{
#if defined(__arm__) || defined(FX_TEST_CANON_ALIASES)
    /* Original EOSM/DIGIC V mem_defs.h. Do not confuse cache/uncache aliases
     * with separately owned storage. Other targets need a different adapter. */
    return (uintptr_t)p&~(uintptr_t)0x40000000u;
#else
    return (uintptr_t)p;
#endif
}
static int span(const void*p,size_t bytes)
{return p&&bytes&&(uintptr_t)p<=UINTPTR_MAX-bytes&&address(p)<=UINTPTR_MAX-bytes;}
static int overlap(const void*a,size_t an,const void*b,size_t bn)
{uintptr_t x=address(a),y=address(b);return x<=y?y-x<an:x-y<bn;}
static int valid(const fx_mirror_raw*m)
{
    if(!m||m->magic!=0x524d4952u)return 0;
    int edges=m->params.value[0]==FX_MIRROR_EDGES;
    if(edges?!fx_edges_valid(&m->params):(m->params.value[0]>=360||m->params.value[1]<2||m->params.value[1]>12||(m->params.value[1]&1)||
        m->params.value[2]>1000||m->params.value[3]>1000))return 0;
    for(unsigned i=0;!edges&&i<m->params.value[1];i++){
        const fx_mirror_raw_sector*s=&m->sector[i];
        if(s->x < -16384||s->x>16384||s->y < -16384||s->y>16384||
            s->a < -16384||s->a>16384||s->b < -16384||s->b>16384||
            s->c < -16384||s->c>16384||s->d < -16384||s->d>16384)return 0;
    }
    const gl_frame*f=&m->source;
    if(!edges&&(m->center_x<0||m->center_x>(int)(m->cells_w-1)*2||
                m->center_y<0||m->center_y>(int)(m->cells_h-1)*2))return 0;
    return f->width>=8&&f->width<=8192&&!(f->width&7)&&f->height>=2&&f->height<=8192&&
        (uint64_t)f->width*f->height<=30000000&&f->format<=GL_CANON14&&
        f->black<f->white&&f->white<=16383&&
        f->pitch>=(f->format==GL_CANON14?f->width*14/8:f->width*2)&&
        (uint64_t)f->pitch*f->height<=f->size&&span(f->data,f->size)&&
        !overlap(m,sizeof(*m),f->data,f->size)&&
        !((m->left|m->top|m->right|m->bottom)&1)&&
        m->right>m->left&&m->bottom>m->top&&m->right<=f->width&&m->bottom<=f->height&&
        m->cells_w==(m->right-m->left)/2&&m->cells_h==(m->bottom-m->top)/2;
}
typedef uint16_t sensor16 __attribute__((may_alias));
static unsigned word(const unsigned char*p)
{
    if(!((uintptr_t)p&1))return *(const sensor16*)p;
    return p[0]|((unsigned)p[1]<<8);
}
static void put(unsigned char*p,unsigned v)
{
    if(!((uintptr_t)p&1)){*(sensor16*)p=(uint16_t)v;return;}
    p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);
}
static unsigned read14(const unsigned char*row,unsigned x)
{
    unsigned bit=x*14,shift=bit&15;row+=(bit/16)*2;unsigned a=word(row);
    return shift<=2?(a>>(2-shift))&16383:((a<<(shift-2))|(word(row+2)>>(18-shift)))&16383;
}
static void set14(unsigned char*row,unsigned x,unsigned v)
{
    unsigned bit=x*14,shift=bit&15;row+=(bit/16)*2;unsigned a=word(row);
    if(shift<=2){unsigned at=2-shift;put(row,(a&~(16383u<<at))|(v<<at));}
    else{
        unsigned low=16-shift,mask=(1u<<low)-1;
        put(row,(a&~mask)|(v>>(14-low)));
        unsigned b=word(row+2),at=low+2;mask=(1u<<(14-low))-1;
        put(row+2,(b&~(mask<<at))|((v&mask)<<at));
    }
}
/* Eight samples in seven words, reversing complete two-sample Bayer cells.
 * Read the entire source group before any destination store. Both green
 * phases stay in their original column/row parity. No neighboring fields are
 * read/modified as they are with repeated scalar set14 operations. */
static void reverse_cells(unsigned char*d,const unsigned char*s)
{
#if defined(__arm__) && !defined(__thumb__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    if(!(((uintptr_t)d|(uintptr_t)s)&1)){
        /* Canon groups are 14 bytes: successive groups alternate word phase.
         * Every access below is naturally aligned and stays inside the group.
         * may_alias permits packed byte buffers without C effective-type UB.
         * Load all seven words before publishing any output. Uncached sensor
         * traffic uses four loads/stores per group rather than fourteen bytes. */
        typedef uint16_t alias16 __attribute__((may_alias));
        typedef uint32_t alias32 __attribute__((may_alias));
        unsigned a,b,c,e,f,g,h;
        if(!((uintptr_t)s&3)){
            unsigned u=((const alias32*)s)[0],v=((const alias32*)s)[1],w=((const alias32*)s)[2];
            a=u&65535;b=u>>16;c=v&65535;e=v>>16;f=w&65535;g=w>>16;h=((const alias16*)s)[6];
        }else{
            a=((const alias16*)s)[0];
            unsigned u=((const alias32*)(s+2))[0],v=((const alias32*)(s+2))[1],w=((const alias32*)(s+2))[2];
            b=u&65535;c=u>>16;e=v&65535;f=v>>16;g=w&65535;h=w>>16;
        }
        /* Reverse four 28-bit Bayer pairs directly in the 112-bit stream. */
        uint16_t o0=(uint16_t)((g<<4)|(h>>12));
        uint16_t o1=(uint16_t)((h<<4)|((e>>4)&15));
        uint16_t o2=(uint16_t)((e<<12)|(f>>4));
        uint16_t o3=(uint16_t)((f<<12)|((g>>4)&0xf00)|((b<<4)&0xf0)|(c>>12));
        uint16_t o4=(uint16_t)((c<<4)|(e>>12));
        uint16_t o5=(uint16_t)(((e<<4)&0xf000)|(a>>4));
        uint16_t o6=(uint16_t)((a<<12)|(b>>4));
        if(!((uintptr_t)d&3)){
            ((alias32*)d)[0]=o0|((uint32_t)o1<<16);((alias32*)d)[1]=o2|((uint32_t)o3<<16);
            ((alias32*)d)[2]=o4|((uint32_t)o5<<16);((alias16*)d)[6]=o6;
        }else{
            ((alias16*)d)[0]=o0;((alias32*)(d+2))[0]=o1|((uint32_t)o2<<16);
            ((alias32*)(d+2))[1]=o3|((uint32_t)o4<<16);((alias32*)(d+2))[2]=o5|((uint32_t)o6<<16);
        }
        return;
    }
#endif
    unsigned a=word(s),b=word(s+2),c=word(s+4),e=word(s+6),f=word(s+8),g=word(s+10),h=word(s+12);
    unsigned p0=a>>2,p1=((a&3)<<12)|(b>>4),p2=((b&15)<<10)|(c>>6),p3=((c&63)<<8)|(e>>8);
    unsigned p4=((e&255)<<6)|(f>>10),p5=((f&1023)<<4)|(g>>12),p6=((g&4095)<<2)|(h>>14),p7=h&16383;
    put(d,(p6<<2)|(p7>>12));put(d+2,(p7<<4)|(p4>>10));put(d+4,(p4<<6)|(p5>>8));
    put(d+6,(p5<<8)|(p2>>6));put(d+8,(p2<<10)|(p3>>4));
    put(d+10,(p3<<12)|(p0>>2));put(d+12,(p0<<14)|p1);
}
static void copy_region(unsigned char*d,const unsigned char*s,unsigned left,unsigned right)
{
    unsigned x=left;
    while(x<right&&(x&7)){set14(d,x,read14(s,x));x++;}
    unsigned end=right&~7u;
    if(end>x){
        size_t bytes=(end-x)*14u/8u;unsigned char*out=d+x*14u/8u;const unsigned char*in=s+x*14u/8u;
        /* The EOS M crop's first full group is commonly only two-byte
         * aligned. Peel equal misalignment before the existing ARM bursts. */
        if(!(((uintptr_t)out^(uintptr_t)in)&3))while(bytes&&((uintptr_t)out&3)){*out++=*in++;bytes--;}
        fx_copy_cpu(out,in,bytes);x=end;
    }
    while(x<right){set14(d,x,read14(s,x));x++;}
}
#ifndef FX_SENSOR_AXIAL_ONLY
int fx_mirror_sensor_pack(const fx_mirror_raw*m,void*destination,size_t capacity,
    unsigned pitch,uint16_t*work,size_t samples)
{
    if(!valid(m)||pitch<m->source.width*14/8||
        (uint64_t)pitch*m->source.height>capacity||!span(destination,capacity)||
        samples<m->source.width||samples>SIZE_MAX/2||!span(work,samples*2)||((uintptr_t)work&1)||
        overlap(destination,capacity,m->source.data,m->source.size)||overlap(destination,capacity,m,sizeof(*m))||
        overlap(work,samples*2,m->source.data,m->source.size)||overlap(work,samples*2,m,sizeof(*m))||
        overlap(work,samples*2,destination,capacity)||
        (m->stop&&(overlap((const void*)m->stop,sizeof(*m->stop),destination,capacity)||overlap((const void*)m->stop,sizeof(*m->stop),work,samples*2))))return FX_INVALID;
    unsigned width=m->source.width;
    for(unsigned y=0;y<m->source.height;y++){
        int result=fx_mirror_raw_row(m,y,work,samples);if(result)return result;
        for(unsigned x=0;x<width;x++)if(work[x]>16383)return FX_INVALID;
        unsigned char*out=(unsigned char*)destination+(size_t)y*pitch;
        for(unsigned x=0;x<width;x+=8){
            const uint16_t*s=work+x;
            put(out+0,(unsigned)s[0]<<2|s[1]>>12);put(out+2,(unsigned)s[1]<<4|s[2]>>10);
            put(out+4,(unsigned)s[2]<<6|s[3]>>8);put(out+6,(unsigned)s[3]<<8|s[4]>>6);
            put(out+8,(unsigned)s[4]<<10|s[5]>>4);put(out+10,(unsigned)s[5]<<12|s[6]>>2);
            put(out+12,(unsigned)s[6]<<14|s[7]);out+=14;
        }
    }
    return FX_OK;
}

size_t fx_mirror_sensor_inplace_size(const fx_mirror_raw*m)
{
    if(!valid(m)||m->source.format!=GL_CANON14)return 0;
    return ((size_t)m->cells_w*m->cells_h+4)/5;
}
static int planning_stop(volatile int*stop)
{
#ifdef FX_SENSOR_TEST_HOOK
    FX(sensor_plan_hook)();
#endif
    return stop&&*stop;
}
static unsigned parent(const fx_mirror_raw*m,unsigned id)
{
    unsigned xy[2]={0,0};
    /* The immutable local context has stop=NULL. Every cell and closed output
     * was validated before peeling/writing; these point preconditions cannot
     * change during either pass. This call cannot fail under that contract. */
    (void)fx_mirror_raw_point(m,m->left+(id%m->cells_w)*2,m->top+(id/m->cells_w)*2,xy);
    return ((xy[1]-m->top)/2)*m->cells_w+(xy[0]-m->left)/2;
}
/* A complete horizontal Bayer pair is 28 contiguous bits. Touch each packed
 * word only once, retaining the neighboring cell's shared boundary bits. */
static unsigned read_pair(const unsigned char*row,unsigned x)
{
    unsigned bit=x*14,shift=bit&15;row+=(bit/16)*2;
    unsigned a=word(row),b=word(row+2),v;
    if(shift<=4)v=((a<<16)|b)>>(4-shift);
    else v=(a<<(12+shift))|(b<<(shift-4))|(word(row+4)>>(20-shift));
    return v&0x0fffffffu;
}
static void write_pair(unsigned char*row,unsigned x,unsigned v)
{
    unsigned bit=x*14,shift=bit&15;row+=(bit/16)*2;
    unsigned keep=shift?(word(row)&(0xffffu<<(16-shift))):0;
    put(row,keep|(v>>(12+shift)));
    if(shift<=4){
        unsigned low=4-shift,mask=(1u<<low)-1;
        put(row+2,(word(row+2)&mask)|(v<<low));
    }else{
        put(row+2,v>>(shift-4));
        unsigned low=20-shift,mask=(1u<<low)-1;
        put(row+4,(word(row+4)&mask)|(v<<low));
    }
}
static void read_cell_at(const fx_mirror_raw*m,unsigned cx,unsigned cy,unsigned v[4])
{
    unsigned x=m->left+cx*2,y=m->top+cy*2;
    const unsigned char*row=(const unsigned char*)m->source.data+(size_t)y*m->source.pitch;
    unsigned a=read_pair(row,x),b=read_pair(row+m->source.pitch,x);
    v[0]=a>>14;v[1]=a&16383;v[2]=b>>14;v[3]=b&16383;
}
static void read_cell(const fx_mirror_raw*m,unsigned id,unsigned v[4])
{read_cell_at(m,id%m->cells_w,id/m->cells_w,v);}
static void write_cell_at(const fx_mirror_raw*m,unsigned cx,unsigned cy,const unsigned v[4])
{
    unsigned x=m->left+cx*2,y=m->top+cy*2;
    unsigned char*row=(unsigned char*)m->source.data+(size_t)y*m->source.pitch;
    write_pair(row,x,(v[0]<<14)|v[1]);
    write_pair(row+m->source.pitch,x,(v[2]<<14)|v[3]);
#ifdef FX_SENSOR_TEST_HOOK
    FX(sensor_write_hook)();
#endif
}
static void write_cell(const fx_mirror_raw*m,unsigned id,const unsigned v[4])
{write_cell_at(m,id%m->cells_w,id/m->cells_w,v);}
static unsigned population(unsigned v)
{
    v=v-((v>>1)&0x55555555u);
    v=(v&0x33333333u)+((v>>2)&0x33333333u);
    v=(v+(v>>4))&0x0f0f0f0fu;
    return (v*0x01010101u)>>24;
}
typedef struct {unsigned x,y,end,sx,sy;int px,py,dx,dy;} sensor_cursor;
static int sensor_round(int n){return n>=0?(n+16384)/32768:-((-n+16384)/32768);}
static unsigned sensor_fold(int value,unsigned extent)
{
    if(extent<=1)return 0;
    unsigned n=(unsigned)(value<0?-value:value),period=(extent-1)*2;
    if(n>=period)n%=period;
    return n<extent?n:period-n;
}
/* Advance whole Bayer cells through constant-sector runs. Intersect each ray
 * once per run instead of choosing sectors and dividing pixel IDs millions
 * of times. Q14 additions and the exact signed rounding match raw_point. */
static unsigned scan_next(const fx_mirror_raw*m,sensor_cursor*c,int mapped)
{
    if(m->params.value[0]==FX_MIRROR_EDGES){
        unsigned result=mapped?parent(m,c->y*m->cells_w+c->x):0;
        if(mapped){c->sx=result%m->cells_w;c->sy=result/m->cells_w;}
        if(++c->x==m->cells_w){c->x=0;c->y++;}return result;
    }
    if(c->x==c->end){
        int dx=(int)c->x*2-m->center_x,dy=(int)c->y*2-m->center_y;
        unsigned count=m->params.value[1],sector=0,end=m->cells_w;
        if(count==2)sector=m->sector[0].x*dy-m->sector[0].y*dx<0?1:0;
        else for(unsigned i=0;i<count;i++){
            unsigned next=i+1==count?0:i+1;
            if(m->sector[i].x*dy-m->sector[i].y*dx>=0&&
               m->sector[next].x*dy-m->sector[next].y*dx<0){sector=i;break;}
        }
        for(unsigned i=0;i<count;i++){
            int dot=m->sector[i].x*dy-m->sector[i].y*dx,slope=-2*m->sector[i].y;
            unsigned distance=0;
            if(slope>0&&dot<0)distance=(unsigned)(-dot+slope-1)/(unsigned)slope;
            else if(slope<0&&dot>=0)distance=(unsigned)dot/(unsigned)-slope+1;
            if(distance&&distance<end-c->x)end=c->x+distance;
        }
        const fx_mirror_raw_sector*s=&m->sector[sector];
        c->px=m->center_x*16384+s->a*dx+s->b*dy;
        c->py=m->center_y*16384+s->c*dx+s->d*dy;
        c->dx=s->a*2;c->dy=s->c*2;c->end=end;
    }
    unsigned result=0;
    if(mapped){
        c->sx=sensor_fold(sensor_round(c->px),m->cells_w);
        c->sy=sensor_fold(sensor_round(c->py),m->cells_h);
        result=c->sy*m->cells_w+c->sx;
    }
    c->px+=c->dx;c->py+=c->dy;
    if(++c->x==m->cells_w){c->x=0;c->end=0;c->y++;}
    return result;
}
/* Most source pixels lie in the unchanged wedge. Save only referenced cells
 * outside that fixed set, so all output cells can be written in one pass.
 * The existing scratch bounds this optimization; unusual geometries that do
 * not fit fall back to graph ordering before any sensor byte is changed. */
static int sparse_copy(const fx_mirror_raw*m,void*workspace,size_t size,volatile int*stop)
{
    unsigned count=m->cells_w*m->cells_h,words=(count+31)/32,blocks=(words+7)/8;
    size_t tables=((size_t)words+blocks+1)*4;
    if(((uintptr_t)workspace&3)||tables>size)return 4;
    uint32_t*bits=workspace,*rank=bits+words;
    uint16_t*saved=(uint16_t*)(rank+blocks+1);
    memset(bits,0,words*4);
    sensor_cursor cursor={0};
    for(unsigned i=0;i<count;i++){
        if(!(i&511)&&planning_stop(stop))return FX_CANCEL;
        unsigned source=scan_next(m,&cursor,1);
        if(source>=count)return FX_INVALID;
        bits[source/32]|=1u<<(source&31);
    }
    unsigned total=0;
    memset(&cursor,0,sizeof(cursor));
    for(unsigned i=0;i<count;i++){
        if(!(i&511)&&planning_stop(stop))return FX_CANCEL;
        unsigned mask=1u<<(i&31);
        int referenced=(bits[i/32]&mask)!=0;
        unsigned source=scan_next(m,&cursor,referenced);
        if(referenced&&source==i)bits[i/32]&=~mask;
    }
    for(unsigned block=0;block<blocks;block++){
        rank[block]=total;
        for(unsigned w=block*8;w<words&&w<(block+1)*8;w++)total+=population(bits[w]);
    }
    rank[blocks]=total;
#ifdef FX_SENSOR_PROFILE
    FX(sensor_profile)(total,total>(size-tables)/8);
#endif
    if(total>(size-tables)/8)return 4;
    unsigned at=0;
    for(unsigned w=0;w<words;w++){
        if(!(w&15)&&planning_stop(stop))return FX_CANCEL;
        unsigned mask=bits[w];
        while(mask){
            unsigned i=w*32+(unsigned)__builtin_ctz(mask);mask&=mask-1;
            unsigned v[4];read_cell(m,i,v);
            for(unsigned j=0;j<4;j++)saved[at++]=(uint16_t)v[j];
        }
    }
    if(planning_stop(stop))return FX_CANCEL;
    /* All non-fixed sources are now private. Fixed source cells are skipped
     * below, including their exact shared-word fields. No cancellation or
     * failure after the first write, as in the graph-ordered path. */
    memset(&cursor,0,sizeof(cursor));
    for(unsigned i=0;i<count;i++){
        unsigned dx=cursor.x,dy=cursor.y;
        unsigned source=scan_next(m,&cursor,1);if(source==i)continue;
        unsigned v[4],word_index=source/32,mask=1u<<(source&31);
        if(bits[word_index]&mask){
            unsigned block=word_index/8,index=rank[block];
            for(unsigned w=block*8;w<word_index;w++)index+=population(bits[w]);
            index+=population(bits[word_index]&(mask-1));
            for(unsigned j=0;j<4;j++)v[j]=saved[index*4+j];
        }else read_cell_at(m,cursor.sx,cursor.sy,v);
        write_cell_at(m,dx,dy,v);
    }
    return FX_OK;
}
static int peel(const fx_mirror_raw*m,unsigned char*bits,unsigned count,int writing,volatile int*stop)
{
    for(;;){
        /* 0: unfinished/unreferenced; 1: processed; 2: has a remaining
         * consumer. Processed/reference combination collapses to processed. */
        if(!writing&&planning_stop(stop))return FX_CANCEL;
        trit_clear_refs(bits,count);
        sensor_cursor cursor={0};
        for(unsigned i=0;i<count;i++){
            if(!writing&&!(i&511)&&planning_stop(stop))return FX_CANCEL;
            int remaining=trit_state(bits,i)!=1;
            unsigned source=scan_next(m,&cursor,remaining);
            if(remaining)trit_mark(bits,source,2);
        }
        unsigned removed=0;
        memset(&cursor,0,sizeof(cursor));
        for(unsigned i=0;i<count;i++){
            if(!writing&&!(i&511)&&planning_stop(stop))return FX_CANCEL;
            int leaf=!trit_state(bits,i);unsigned dx=cursor.x,dy=cursor.y;
            (void)scan_next(m,&cursor,writing&&leaf);
            if(leaf){
                if(writing){unsigned v[4];read_cell_at(m,cursor.sx,cursor.sy,v);write_cell_at(m,dx,dy,v);}
                trit_mark(bits,i,1);removed++;
            }
        }
        if(!removed)return FX_OK;
    }
}
int fx_mirror_sensor_inplace(const fx_mirror_raw*m,void*owned,size_t capacity,void*workspace,size_t work_size)
{
    size_t needed=fx_mirror_sensor_inplace_size(m);
    if(!needed||owned!=m->source.data||capacity<m->source.size||!span(owned,capacity)||
        !span(workspace,work_size)||work_size<needed||overlap(owned,capacity,m,sizeof(*m))||
        overlap(workspace,work_size,owned,capacity)||overlap(workspace,work_size,m,sizeof(*m))||
        (m->stop&&(overlap((const void*)m->stop,sizeof(*m->stop),owned,capacity)||
                   overlap((const void*)m->stop,sizeof(*m->stop),workspace,work_size))))return FX_INVALID;
    fx_mirror_raw map=*m;volatile int*stop=map.stop;map.stop=0;
    if(!valid(&map)||overlap(workspace,work_size,&map,sizeof(map)))return FX_INVALID;
    if(planning_stop(stop))return FX_CANCEL;
    if((map.params.value[0]==0||map.params.value[0]==90)&&
       (map.params.value[1]==2||map.params.value[1]==4)&&
       map.params.value[2]==500&&map.params.value[3]==500)
        return fx_mirror_sensor_axial(&map,owned,capacity);
    int sparse=sparse_copy(&map,workspace,work_size,stop);
    if(sparse!=4)return sparse;
    unsigned count=map.cells_w*map.cells_h;unsigned char*bits=workspace;
    /* Validate complete mapping closure and Bayer-cell phase before any sample
     * read/write. Geometry is then immutable; point evaluation cannot fail. */
    sensor_cursor cursor={0};
    for(unsigned i=0;i<count;i++){
        if(!(i&511)&&planning_stop(stop))return FX_CANCEL;
        if(scan_next(&map,&cursor,1)>=count)return FX_INVALID;
    }
    memset(bits,0,needed);
    int result=peel(&map,bits,count,0,stop);if(result)return result;
    if(planning_stop(stop))return FX_CANCEL;
    /* No cancellable/error-returning operations after this point. Caller must
     * keep all Canon consumers excluded until the complete transform returns. */
    memset(bits,0,needed);(void)peel(&map,bits,count,1,0);
    for(unsigned start=0;start<count;start++)if(trit_state(bits,start)!=1){
        unsigned next=parent(&map,start);
        if(next==start){trit_mark(bits,start,1);continue;}
        unsigned saved[4];read_cell(&map,start,saved);unsigned current=start;
        for(;;){
            next=parent(&map,current);
            if(next==start){write_cell(&map,current,saved);trit_mark(bits,current,1);break;}
            unsigned v[4];read_cell(&map,next,v);write_cell(&map,current,v);trit_mark(bits,current,1);current=next;
        }
    }
    return FX_OK;
}
#endif
static int sensor_edges(const fx_mirror_raw*m,void*owned)
{
    if(m->stop&&*m->stop)return FX_CANCEL;
    unsigned w=m->cells_w,h=m->cells_h,mask=m->params.value[1];
    unsigned border=w/m->params.value[2],root_begin=(mask&1)?border:0,root_end=w-((mask&2)?border:0);
    for(unsigned y=0;y<h;y++){
        unsigned sy=fx_edge_coordinate(y,h,m->params.value[3],mask&4,mask&8);
        unsigned char*d=(unsigned char*)owned+(size_t)(m->top+y*2)*m->source.pitch;
        const unsigned char*s=(const unsigned char*)owned+(size_t)(m->top+sy*2)*m->source.pitch;
        if(sy!=y){
            for(unsigned row=0;row<2;row++)copy_region(d+row*m->source.pitch,s+row*m->source.pitch,m->left+root_begin*2,m->left+root_end*2);
#ifdef FX_SENSOR_TEST_HOOK
            FX(sensor_write_hook)();
#endif
        }
        for(unsigned side=0;side<2;side++){
            if(!(mask&(1u<<side)))continue;
            unsigned end=side?w:border;
            for(unsigned x=side?w-border:0;x<end;){
                unsigned rx=m->left+fx_edge_coordinate(x,w,m->params.value[2],mask&1,mask&2)*2,dx=m->left+x*2;
                if(!(dx&7)&&x+4<=end&&!((rx-6)&7)){
                    for(unsigned row=0;row<2;row++)reverse_cells(d+row*m->source.pitch+dx*14u/8u,s+row*m->source.pitch+(rx-6)*14u/8u);
                    x+=4;
                }else{
                    unsigned a=read14(s,rx),b=read14(s,rx+1),c=read14(s+m->source.pitch,rx),e=read14(s+m->source.pitch,rx+1);
                    set14(d,dx,a);set14(d,dx+1,b);set14(d+m->source.pitch,dx,c);set14(d+m->source.pitch,dx+1,e);x++;
                }
#ifdef FX_SENSOR_TEST_HOOK
                FX(sensor_write_hook)();
#endif
            }
        }
    }
    return FX_OK;
}
int fx_mirror_sensor_axial(const fx_mirror_raw*m,void*owned,size_t capacity)
{
    if(!valid(m)||m->source.format!=GL_CANON14||owned!=m->source.data||capacity<m->source.size||
        (m->params.value[0]!=FX_MIRROR_EDGES&&((m->params.value[0]!=0&&m->params.value[0]!=90)||
        (m->params.value[1]!=2&&m->params.value[1]!=4)||m->params.value[2]!=500||m->params.value[3]!=500)))
        return FX_INVALID;
    if(m->params.value[0]==FX_MIRROR_EDGES)return sensor_edges(m,owned);
    if(m->stop&&*m->stop)return FX_CANCEL;
    unsigned w=m->cells_w,h=m->cells_h;int vertical=m->params.value[0]==90,four=m->params.value[1]==4;
    for(unsigned y=0;y<h;y++){
        unsigned sy=(!vertical||four)&&(y<h-1-y)?h-1-y:y;
        unsigned dy=m->top+y*2,ry=m->top+sy*2;
        unsigned char*d=(unsigned char*)owned+(size_t)dy*m->source.pitch;
        const unsigned char*s=(const unsigned char*)owned+(size_t)ry*m->source.pitch;
        /* Fixed horizontal cells can copy whole packed spans when their row
         * comes from the fixed vertical half. Fixed rows need no such writes. */
        unsigned root_begin=vertical?0:four?w/2:0,root_end=vertical?(w+1)/2:w;
        if(sy!=y){
            for(unsigned row=0;row<2;row++)copy_region(d+row*m->source.pitch,s+row*m->source.pitch,
                m->left+root_begin*2,m->left+root_end*2);
#ifdef FX_SENSOR_TEST_HOOK
            FX(sensor_write_hook)();
#endif
        }
        if(!vertical&&!four)continue;
        unsigned end=vertical?w:w/2;
        for(unsigned x=vertical?(w+1)/2:0;x<end;){
            unsigned dx=m->left+x*2,rx=m->left+(w-1-x)*2;
            if(!(dx&7)&&x+4<=end&&!((rx-6)&7)){
                for(unsigned row=0;row<2;row++)reverse_cells(d+row*m->source.pitch+dx*14u/8u,s+row*m->source.pitch+(rx-6)*14u/8u);
#ifdef FX_SENSOR_TEST_HOOK
                FX(sensor_write_hook)();
#endif
                x+=4;continue;
            }
            /* All four reads precede writes; bit-field updates preserve neighbors,
             * including fixed cells sharing the same 16-bit word. */
            unsigned a=read14(s,rx),b=read14(s,rx+1),c=read14(s+m->source.pitch,rx),e=read14(s+m->source.pitch,rx+1);
            set14(d,dx,a);set14(d,dx+1,b);set14(d+m->source.pitch,dx,c);set14(d+m->source.pitch,dx+1,e);
#ifdef FX_SENSOR_TEST_HOOK
        FX(sensor_write_hook)();
#endif
            x++;
        }
    }
    return FX_OK;
}
