"""Execute the actual packed-RAW backend on ARMv5TE, compare host output."""
from benchmark_camera_pipeline import ROOT,BASE,REV,CASES
from benchmark_preview import arm_runner
import argparse,hashlib,json,subprocess
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--variant',choices=['both','current'],default='both');args=parser.parse_args()
    folder=ROOT/'build/sensor-arm';folder.mkdir(parents=True,exist_ok=True)
    baseline=folder/'baseline';baseline.mkdir(exist_ok=True)
    for name in ['fx_mirror_sensor.c','fx_mirror_raw.c']:
        (baseline/name).write_bytes(subprocess.check_output(['git','show',REV+':modules/fx_common/'+name],cwd=ROOT))
    results={'baseline_revision':REV,'sensor_fixture':[320,212],'active_pixels':[304,204],
             'scratch_scaled_to_full_sensor':True,'camera_capture_time_verified':False,'variants':{}}
    for variant,directory in [('baseline',baseline),('current',ROOT/'modules/fx_common')]:
        if variant=='baseline' and args.variant=='current':
            results['variants']['baseline']=json.loads((folder/'comparison.json').read_text())['variants']['baseline'];continue
        out=folder/variant;out.mkdir(exist_ok=True);relative=out.relative_to(ROOT).as_posix();src=directory.relative_to(ROOT).as_posix()
        flags=['-std=c99','-O2','-Wall','-Wextra','-Werror','-DFX_PREFIX=bench','-Imodules/fx_common','-Icore',*(['-DSENSOR_CURRENT'] if variant=='current' else [])]
        sources=[src+'/fx_mirror_sensor.c',src+'/fx_mirror_raw.c','tests/sensor_arm_benchmark.c']
        subprocess.run(BASE+['gcc',*flags,*sources,'-o',relative+'/host'],check=True)
        subprocess.run(BASE+['arm-none-eabi-gcc',*flags,'-march=armv5te','-marm','-mlong-calls','-ffreestanding','-fno-builtin','-fno-optimize-sibling-calls','-fno-tree-loop-distribute-patterns','-nostdlib','-DSENSOR_ARM',*sources,'-Wl,-Ttext=0x10000,--entry=preview_arm_test','-o',relative+'/sensor.elf','-lgcc'],check=True)
        execute=arm_runner(out/'sensor.elf');cases={};results['variants'][variant]=cases
        for name,count,angle,x,y in CASES+[('center-extreme',6,359,1000,0)]:
            config=[angle,count,x,y];path=relative+'/'+name+'.raw'
            subprocess.run(BASE+[relative+'/host',path,*map(str,config)],check=True)
            measured=execute(config);assert measured['sha256']==hashlib.sha256((ROOT/path).read_bytes()).hexdigest()
            cases[name]=measured;print(variant,name,json.dumps(measured),flush=True)
    results['comparison']={}
    results['current_source_sha256']={name:hashlib.sha256((ROOT/'modules/fx_common'/name).read_bytes()).hexdigest() for name in ['fx_mirror_sensor.c','fx_mirror_raw.c']}
    for name in results['variants']['baseline']:
        a,b=[results['variants'][v][name] for v in ['baseline','current']];assert a['sha256']==b['sha256']
        results['comparison'][name]={'pixel_identical':True,'ARM_instruction_reduction':a['instructions']/b['instructions']}
    (folder/'comparison.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(results['comparison'],indent=2))
if __name__=='__main__':main()
