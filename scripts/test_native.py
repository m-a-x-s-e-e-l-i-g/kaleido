"""Run real C pixel/sensor code in the same pinned builder as the module."""
from pathlib import Path
import subprocess, sys
root=Path(__file__).resolve().parents[1]
(root/'build').mkdir(exist_ok=True)
base=['docker','run','--rm','--mount',f'type=bind,source={root},target=/work','-w','/work','glitchlock-ml-builder:bookworm']
only=sys.argv[sys.argv.index("--only")+1] if "--only" in sys.argv else "all"
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
