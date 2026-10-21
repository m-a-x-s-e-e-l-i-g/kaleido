# Kaleido 0.1.0 · Installation and shooting

For the **original Canon EOS M**, **Canon firmware 2.0.2**, and **Magic Lantern dev 2025-06-20 / EOSM.202** (source `ed3e7c0`). Other cameras and Magic Lantern builds are unsupported.

Max Seelig reports that the preceding **0.1.0-rc4** passed physical-camera testing and works well. The first public release preserves that implementation; only its version label and module documentation changed. The build verifies this against the exact tested archive. See [the test report](camera-test-report.json).

## Install

Start with a working Magic Lantern installation. [Magic Lantern](https://www.magiclantern.fm/) is a free, independent extension that loads from the memory card alongside Canon's software. Follow its [EOS M installation instructions](https://builds.magiclantern.fm/EOSM.html) before installing Kaleido.

1. Download and unzip **KALEIDO-0.1.0-EOSM202.zip** from [the official release](https://github.com/m-a-x-s-e-e-l-i-g/kaleido/releases/tag/v0.1.0).
2. Power the camera **OFF**. Copy only `ML/MODULES/kaleido.mo` into the same folder on the card. Keep the existing `autoexec.bin`, firmware and Magic Lantern core.
3. Boot, open ML's **Modules** tab and disable all other MAXmade effects: **mirror, glock, slitscan, feedback, fragment and diffract**, if present. Enable **kaleido**, then fully power OFF and restart.
4. Select **A+**, **Auto ISO**, **Canon RAW** and **single-shot** drive. Use **native 1× LCD** and **Global Draw ON**.
5. Open **Shoot → Kaleido**. Turn **Live effect ON**, choose the pattern and close the menu.

A loaded sibling effect blocks Kaleido even if that sibling's effect switch is OFF. Changing module selections requires a restart.

## Compose and shoot

| Control | Values |
| --- | --- |
| Live effect | OFF / ON |
| Segments | 4 / 6 / 12 |
| Rotation | 0–359°, one degree per step |
| Center X / Center Y | 0–100%, 2.5% per step |

Initial settings are OFF, 6 segments, 0° rotation and center 50% / 50%. Settings save automatically to Kaleido's separate checked preference slots; existing rc4 preferences carry over.

Compose in LiveView and use the **normal physical shutter**. Half-press holds the last delivered composition. Release between shots and let processing finish. Kaleido transforms whole Bayer cells in the native Canon CR2 path and leaves optical black intact. Bursts, video, remote triggers and long exposures are outside this release's supported workflow.

At an extreme center, four segments and some angles can leave much of the image unchanged because it already occupies the source quadrant. Try another rotation or move the center inward.

## Troubleshooting

If LiveView does not start, check the exact camera/core version, native 1× display, Global Draw and loaded sibling modules, then restart. Memory availability depends on other enabled modules. After a memory startup failure, a settings change permits one retry when the menu closes.

A capture error stays latched until Live effect is switched OFF/ON. If it persists, restart and [report the error](https://github.com/m-a-x-s-e-e-l-i-g/kaleido/issues), settings and an original CR2. An early guard rejection leaves that shot unprocessed. Once sensor writing starts, the transform finishes before Canon resumes. If a completed hold exceeds the inherited two-second post-hoc budget, the effect stops for following shots; this is not a hard processing deadline. No measured camera FPS or processing-time claim accompanies this release.

To uninstall, power OFF and remove `ML/MODULES/kaleido.mo`; deselect it in Modules if needed. Do not replace or unload a running module.

## Package and credits

The ZIP includes the module, instructions, source, GPL license, exact-core import audit and checksums. `BUILD.json` separates the author's rc4 camera report from automated checks: its `camera_loaded` / `camera_capture_tested` flags remain false for the newly versioned binary, while `same_implementation_as_camera_tested_predecessor` records the verified source comparison.

By **Max Seelig · [MAXmade](https://maxmade.nl/)**. GPL-2.0-or-later. For simpler reflections, try the separate [Mirror Worlds module](https://mirror-worlds.netlify.app/).
