"""rc2 vs current: full sensor time and exact ARM preview/cache-model work.

Cache counts use a cold, 4-way LRU, 32-byte-line write-allocate model. They are
memory-locality evidence, not measured DIGIC cache misses, elapsed time or FPS.
"""
from pathlib import Path
import argparse,hashlib,json,statistics,subprocess
from benchmark_preview import ROOT,BASE,arm_runner
REV='6b6439c'
CASES=[('six-default',6,0,500,500),('six-rotated',6,17,425,575),
       ('twelve-rotated',12,179,500,500),('four-axial',4,0,500,500),('four-rotated',4,17,500,500)]
def run(args):return subprocess.run(args,check=True)
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--only',choices=['all','sensor','preview'],default='all');parser.add_argument('--variant',choices=['both','baseline','current'],default='both');args=parser.parse_args()
    folder=ROOT/'build/camera-performance';folder.mkdir(parents=True,exist_ok=True)
    baseline=folder/'baseline';baseline.mkdir(exist_ok=True)
    for name in ['fx_mirror.c','fx_mirror.h','fx_mirror_edges.h','fx_mirror_raw.c','fx_mirror_sensor.c']:
        (baseline/name).write_bytes(subprocess.check_output(['git','show',REV+':modules/fx_common/'+name],cwd=ROOT))
    result={'baseline_revision':REV,'camera_FPS_verified':False,'camera_capture_time_verified':False,
            'cache_model':'cold 4-way LRU, 32-byte lines, write allocate; 8/16/32 KiB; no timing model',
            'variants':{}}
    for variant,directory in [('baseline',baseline),('current',ROOT/'modules/fx_common')]:
        out=folder/variant;out.mkdir(exist_ok=True);relative=out.relative_to(ROOT).as_posix();src=directory.relative_to(ROOT).as_posix()
        if args.variant not in ['both',variant]:
            result['variants'][variant]=json.loads((out/'result.json').read_text());continue
        flags=['-std=c99','-O2','-Wall','-Wextra','-Werror','-DFX_PREFIX=bench','-I'+src,'-Imodules/fx_common','-Icore']
        results=json.loads((out/'result.json').read_text()) if (out/'result.json').exists() else {}
        result['variants'][variant]=results
        results['source_sha256']={name:hashlib.sha256((directory/name).read_bytes()).hexdigest() for name in ['fx_mirror.c','fx_mirror_raw.c','fx_mirror_sensor.c']}
        if args.only in ['all','sensor']:
            run(BASE+['gcc',*flags,'-DFX_SENSOR_PROFILE','tests/sensor_benchmark.c',src+'/fx_mirror_sensor.c',src+'/fx_mirror_raw.c','-o',relative+'/sensor'])
            results['sensor']={}
            for name,segments,angle,x,y in CASES:
                samples=[]
                for repeat in range(3):
                    path=relative+'/sensor.raw'
                    measurement=json.loads(subprocess.check_output(BASE+[relative+'/sensor',str(segments),str(angle),str(x),str(y),path,*(['2097152'] if variant=='current' else [])],text=True))
                    samples.append(measurement['ms'])
                measurement['median_host_ms']=statistics.median(samples);measurement['samples_host_ms']=samples
                measurement['sha256']=hashlib.sha256((ROOT/path).read_bytes()).hexdigest();(ROOT/path).unlink()
                results['sensor'][name]=measurement;print(variant,'sensor',name,json.dumps(measurement),flush=True)
        if args.only in ['all','preview']:
            run(BASE+['gcc',*flags,'tests/preview_benchmark.c',src+'/fx_mirror.c','-o',relative+'/preview-host'])
            run(BASE+['arm-none-eabi-gcc',*flags,'-march=armv5te','-marm','-mlong-calls','-ffreestanding','-fno-builtin','-fno-optimize-sibling-calls','-fno-tree-loop-distribute-patterns','-nostdlib','-DPREVIEW_ARM',*(['-DPREVIEW_COMMIT'] if variant=='baseline' else []),'tests/preview_benchmark.c',src+'/fx_mirror.c','-Wl,-Ttext=0x10000,--entry=preview_arm_test','-o',relative+'/preview.elf','-lgcc'])
            execute=arm_runner(out/'preview.elf',cache=True);results['preview']={}
            for name,segments,angle,x,y in CASES:
                config=[angle,segments,x,y];path=relative+'/'+name+'.uyvy'
                host=json.loads(subprocess.check_output(BASE+[relative+'/preview-host',path,*map(str,config)],text=True))
                arm=execute(config);assert arm['sha256']==hashlib.sha256((ROOT/path).read_bytes()).hexdigest()
                results['preview'][name]=arm;print(variant,'preview',name,json.dumps(arm),flush=True)
        (out/'result.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8',newline='\n')
    result['comparison']={}
    for kind in ['sensor','preview']:
        if kind not in result['variants']['baseline']:continue
        comparisons={};result['comparison'][kind]=comparisons
        for name,*_ in CASES:
            a,b=[result['variants'][v][kind][name] for v in ['baseline','current']];assert a['sha256']==b['sha256']
            c={'pixel_identical':True};comparisons[name]=c
            if kind=='sensor':c['host_speedup']=a['median_host_ms']/b['median_host_ms']
            else:
                c['ARM_instruction_reduction']=a['instructions']/b['instructions']
                c['modeled_cache_miss_reduction']={size:sum(a['modeled_cache_misses'][size].values())/sum(b['modeled_cache_misses'][size].values()) for size in a['modeled_cache_misses']}
    (folder/(args.only+'.json')).write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(result['comparison'],indent=2),flush=True)
if __name__=='__main__':main()
