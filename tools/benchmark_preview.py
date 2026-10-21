"""Exact before/after pixels and ARMv5TE instruction work. Not camera FPS."""
from pathlib import Path
import argparse,hashlib,json,statistics,struct,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.cache/arm-runtime'))
CASES=[('six-default',0,6,500,500),('six-rotated',17,6,425,575),
       ('twelve-rotated',179,12,500,500),('four-axial',0,4,500,500),('four-shifted',315,4,750,250)]
IMAGE='glitchlock-ml-builder:bookworm'
BASE=['docker','run','--rm','--mount',f'type=bind,source={ROOT},target=/work','-w','/work',IMAGE]
def run(args,**kw):return subprocess.run(args,check=True,**kw)
def compile_variant(variant,source,folder,arm):
    flags=['-std=c99','-O2','-Wall','-Wextra','-Werror','-DFX_PREFIX=bench','-Imodules/fx_common']
    run(BASE+['gcc',*flags,source,'tests/preview_benchmark.c','-o',folder+'/host'])
    if arm:
        run(BASE+['arm-none-eabi-gcc',*flags,'-march=armv5te','-marm','-mlong-calls','-ffreestanding','-fno-builtin','-fno-optimize-sibling-calls','-fno-tree-loop-distribute-patterns','-nostdlib','-DPREVIEW_ARM',source,'tests/preview_benchmark.c','-Wl,-Ttext=0x10000,--entry=preview_arm_test','-o',folder+'/preview.elf','-lgcc'])
def arm_runner(path,cache=False):
    import unicorn as U
    from unicorn import arm_const as A
    from elftools.elf.elffile import ELFFile
    cpu=U.Uc(U.UC_ARCH_ARM,U.UC_MODE_ARM)
    cpu.mem_map(0x10000,0x100000)
    for address,size in [(0x200000,0x100000),(0x300000,0x100000),(0x400000,0x100000),(0x500000,0x1000),(0x600000,0x10000),(0x700000,0x100000)]:cpu.mem_map(address,size)
    with path.open('rb') as f:
        elf=ELFFile(f);entry=elf.header['e_entry']
        for section in elf.iter_sections():
            if section['sh_flags']&2 and section['sh_type']!='SHT_NOBITS':cpu.mem_write(section['sh_addr'],section.data())
    source=bytes((i*37+i//51)&255 for i in range(691200));cpu.mem_write(0x200000,source)
    count=[0];counting=[False]
    def block(uc,pc,size,data):
        if counting[0]:count[0]+=size//4
    caches={}
    def memory(uc,access,address,size,value,data):
        if not counting[0]:return
        category='source' if 0x200000<=address<0x300000 else 'output' if 0x300000<=address<0x400000 else 'lookup' if 0x400000<=address<0x500000 else 'commit' if 0x700000<=address<0x800000 else 'other'
        for kib,model in caches.items():
            sets=kib*1024//(32*4)
            for line in range(address//32,(address+size-1)//32+1):
                ways=model['sets'][line%sets]
                if line in ways:ways.remove(line)
                else:
                    model['misses'][category]+=1
                    if len(ways)==4:ways.pop(0)
                ways.append(line)
    preserved=[A.UC_ARM_REG_R4,A.UC_ARM_REG_R5,A.UC_ARM_REG_R6,A.UC_ARM_REG_R7,A.UC_ARM_REG_R8,A.UC_ARM_REG_R9,A.UC_ARM_REG_R10,A.UC_ARM_REG_R11]
    def call(config,measure=False):
        caches.clear()
        if cache:
            for kib in [8,16,32]:caches[kib]={'sets':[[] for _ in range(kib*1024//128)],'misses':dict.fromkeys(['source','output','lookup','commit','other'],0)}
        cpu.mem_write(0x500000,struct.pack('<5I',*config));cpu.reg_write(A.UC_ARM_REG_CPSR,0x10)
        for index,reg in enumerate(preserved):cpu.reg_write(reg,0xabc00000+index)
        for reg,value in [(A.UC_ARM_REG_R0,0x200000),(A.UC_ARM_REG_R1,0x300000),(A.UC_ARM_REG_R2,0x400000),(A.UC_ARM_REG_R3,0x500000),(A.UC_ARM_REG_SP,0x60ff00),(A.UC_ARM_REG_LR,0xffff0)]:cpu.reg_write(reg,value)
        count[0]=0;counting[0]=measure
        block_hook=cpu.hook_add(U.UC_HOOK_BLOCK,block) if measure else None
        memory_hook=cpu.hook_add(U.UC_HOOK_MEM_READ|U.UC_HOOK_MEM_WRITE,memory) if cache and config[0]==1 else None
        cpu.emu_start(entry,0xffff0,count=200000000);counting[0]=False
        if memory_hook is not None:cpu.hook_del(memory_hook)
        if block_hook is not None:cpu.hook_del(block_hook)
        assert cpu.reg_read(A.UC_ARM_REG_PC)==0xffff0
        assert [cpu.reg_read(r) for r in preserved]==[0xabc00000+i for i in range(8)]
        return cpu.reg_read(A.UC_ARM_REG_R0),count[0]
    def execute(config):
        result,prepare_instructions=call([0,*config],not cache);assert result==0
        result,instructions=call([1,*config],True);assert result==0
        result={'instructions':instructions,'prepare_instructions':prepare_instructions,'sha256':hashlib.sha256(bytes(cpu.mem_read(0x300000,691200))).hexdigest(),'callee_saved_registers_preserved':True}
        if cache:result['modeled_cache_misses']={str(kib)+'KiB':model['misses'] for kib,model in caches.items()}
        if cache:result['prepare_instructions']=None
        return result
    return execute
def main():
    p=argparse.ArgumentParser();p.add_argument('--variant',choices=['baseline','current','both'],default='both');p.add_argument('--host-only',action='store_true');args=p.parse_args()
    output=ROOT/'build/preview-performance';output.mkdir(parents=True,exist_ok=True)
    for variant,source in [('baseline','tests/fixtures/rc1/fx_mirror.c'),('current','modules/fx_common/fx_mirror.c')]:
        if args.variant not in ['both',variant]:continue
        folder=output/variant;folder.mkdir(exist_ok=True);relative=folder.relative_to(ROOT).as_posix()
        compile_variant(variant,source,relative,not args.host_only)
        execute=arm_runner(folder/'preview.elf') if not args.host_only else None
        directory=(ROOT/source).parent
        inputs=[ROOT/source,directory/'fx_mirror.h',directory/'fx_mirror_edges.h',ROOT/'modules/fx_common/fx_copy.h']
        result={'source_sha256':hashlib.sha256((ROOT/source).read_bytes()).hexdigest(),
                'inputs_sha256':{p.relative_to(ROOT).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
                'cases':{},'camera_FPS_verified':False}
        cases=CASES+([(f'{count}-segments-at-180',180,count,500,500) for count in [4,6,12]] if variant=='current' else [])
        for name,*config in cases:
            host=json.loads(subprocess.check_output(BASE+[relative+'/host',relative+'/'+name+'.uyvy',*map(str,config)],text=True))
            host['median_host_ms']=statistics.median(host.pop('samples_host_ms')[1:]);host['sha256']=hashlib.sha256((folder/(name+'.uyvy')).read_bytes()).hexdigest()
            if execute:
                host['arm']=execute(config);assert host['arm']['sha256']==host['sha256']
            result['cases'][name]=host
            print(variant,name,json.dumps(host),flush=True)
        (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
    a,b=[output/n/'result.json' for n in ['baseline','current']]
    if a.exists() and b.exists():
        baseline=json.loads(a.read_text());current=json.loads(b.read_text());comparison={}
        for name in baseline['cases']:
            before,after=baseline['cases'][name],current['cases'][name];assert before['sha256']==after['sha256']
            comparison[name]={'pixel_identical':True,'host_speedup':before['median_host_ms']/after['median_host_ms']}
            if 'arm' in before and 'arm' in after:comparison[name]['ARM_instruction_reduction']=before['arm']['instructions']/after['arm']['instructions']
        report={'baseline_revision':'5255839','comparison':comparison,'camera_FPS_verified':False}
        (output/'comparison.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
