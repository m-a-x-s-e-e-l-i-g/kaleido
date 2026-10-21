"""Run real C pixel/sensor code in the same pinned builder as the module."""
from pathlib import Path
import argparse, subprocess, sys
root=Path(__file__).resolve().parents[1]
(root/'build').mkdir(exist_ok=True)
base=['docker','run','--rm','--mount',f'type=bind,source={root},target=/work','-w','/work','glitchlock-ml-builder:bookworm']
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--only',choices=['all','native','settings','gate','parity','ui','camera'],default='all')
parser.add_argument('--full',action='store_true')
args=parser.parse_args();only=args.only
if only in ("all", "camera"):
    subprocess.run(base+['gcc','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-O2','-DFX_PREFIX=kaleido','-DFX_HOST_TEST','-DFX_MEMORY_HOST_TEST','-DMIRROR_RELEASE','-include','modules/fx_common/fx_namespace.h','-Itests','-Imodules/fx_common','-Icore','-Imodules/kaleido','-Imodules/glock','tests/camera.c','modules/fx_common/fx_camera.c','modules/kaleido/kaleido_driver.c','modules/fx_common/fx_mirror.c','modules/fx_common/fx_pixels.c','modules/glock/gl_format.c','modules/glock/gl_review.c','-o','build/test-camera'],check=True)
    subprocess.run(base+['./build/test-camera'],check=True)
if only in ("all", "native"):
    subprocess.run(base+['gcc','-std=c99','-Wall','-Wextra','-Werror','-O2','-DFX_PREFIX=kaleido','-Imodules/fx_common','-Icore','-Imodules/kaleido','tests/native.c','modules/fx_common/fx_mirror.c','modules/fx_common/fx_mirror_raw.c','modules/fx_common/fx_mirror_sensor.c','-o','build/test-native'],check=True)
    subprocess.run(base+['./build/test-native']+(['--full'] if '--full' in sys.argv else []),check=True)
if only in ("all", "settings"):
    (root/'build/prefs').mkdir(exist_ok=True)
    subprocess.run(base+['gcc','-std=c99','-Wall','-Wextra','-Werror','-O2','-DFX_PREFIX=kaleido','-DFX_SETTINGS_HOST_TEST','-include','modules/fx_common/fx_namespace.h','-Itests','-Imodules/fx_common','-Icore','-Imodules/kaleido','-Imodules/glock','tests/settings.c','modules/kaleido/kaleido_settings.c','modules/glock/gl_format.c','-o','build/test-settings'],check=True)
    subprocess.run(base+['./build/test-settings'],check=True)
if only in ("all", "gate"):
    subprocess.run(base+['gcc','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-O2','-DFX_PREFIX=kaleido','-DFX_GATE_HOST_TEST','-DFX_TRACE_HOST_TEST','-DFX_SENSOR_TEST_HOOK','-DMIRROR_RELEASE','-include','modules/fx_common/fx_namespace.h','-Itests','-Imodules/fx_common','-Icore','-Imodules/kaleido','-Imodules/glock','tests/gate.c','modules/kaleido/kaleido_capture_gate.c','modules/kaleido/kaleido_raw_trace.c','modules/kaleido/kaleido_driver.c','modules/fx_common/fx_mirror.c','modules/fx_common/fx_mirror_raw.c','modules/fx_common/fx_mirror_sensor.c','modules/glock/gl_format.c','-o','build/test-gate'],check=True)
    for scenario in ['repeat','caller','scratch','cancel','shutdown','overlap','planning-cancel']:
        subprocess.run(base+['./build/test-gate',scenario],check=True)
if only in ("all", "parity"):
    # Fixed host layout avoids intermittent PIE/ASAN startup faults in the
    # Windows Docker runtime; both sanitizers and memory checks stay enabled.
    flags=['-std=c99','-Wall','-Wextra','-Werror','-O2','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-Imodules/fx_common']
    subprocess.run(base+['gcc',*flags,'-DFX_PREFIX=baseline','-c','tests/fixtures/rc1/fx_mirror.c','-o','build/preview-baseline.o'],check=True)
    subprocess.run(base+['gcc',*flags,'-no-pie','-DFX_PREFIX=kaleido','tests/preview_parity.c','modules/fx_common/fx_mirror.c','build/preview-baseline.o','-o','build/test-preview-parity'],check=True)
    subprocess.run(base+['env','ASAN_OPTIONS=abort_on_error=1:handle_segv=0','./build/test-preview-parity'],check=True)
if only in ("all", "ui"):
    subprocess.run(base+['gcc','-std=c99','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-const-variable','-Wno-unused-parameter','-O2','-DFX_PREFIX=kaleido','-DFX_MIRROR_UI_HOST_TEST','-include','modules/fx_common/fx_namespace.h','-Itests','-Imodules/fx_common','-Imodules/glock','-Icore','tests/ui.c','-o','build/test-ui'],check=True)
    subprocess.run(base+['./build/test-ui'],check=True)
