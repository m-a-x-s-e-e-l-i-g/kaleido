# Kaleido 0.1.0-rc4 · EOS M test candidate

This is an independently built module, not a replacement firmware or Magic Lantern core. It has not been loaded or photographed with on a physical camera during this implementation.

rc4 fixes memory pressure exposed while changing segments. Preview lookup and RAW scratch share a single exclusive arena, saving 740,516 bytes. Changing 4/6/12 segments does not allocate or restart the effect. After a failed memory startup, changing a setting permits one retry when the menu closes. Other capture faults still require OFF/ON. Existing preferences need no reset.

The camera prefers one 2 MiB private arena, keeping the 1.5 MiB allocator reserve. If that arena does not fit or its allocation fails, it retries the prior 895,796-byte minimum. There is no second preview-workspace allocation. RAW processing owns the arena exclusively; LiveView rebuilds its lookup after release. Full-resolution preview, Canon14 precision and the rc3 pixel-processing optimizations are preserved. Geometry that cannot fit sparse snapshots uses the bounded graph fallback and can still take longer. No complete RAW snapshot is allocated.

## Install

Required target: original Canon EOS M, Canon firmware 2.0.2, ML dev 2025-06-20 / EOSM.202. Start with Magic Lantern already installed.

1. Power OFF. Copy only the ZIP's `ML/MODULES/kaleido.mo` into that folder on your card.
2. Boot, open ML's Modules tab, disable **mirror, glock, slitscan, feedback, fragment and diffract** if present. Enable **kaleido**. Power OFF and restart.
3. Select A+, Auto ISO, Canon RAW and single-shot drive. Use native 1× LCD and Global Draw ON.
4. Open Shoot → Kaleido. Turn Live effect ON, then close the menu. Initial defaults are 6 segments, rotation 0°, center 50% / 50%.

Kaleido refuses to run with a loaded sibling MAXmade effect module, even when that sibling's own effect switch is OFF. A restart is required after changing the Modules selections.

## Controls and shooting

| Control | Values |
| --- | --- |
| Live effect | OFF / ON |
| Segments | 4 / 6 / 12 |
| Rotation | 0–359°, one degree per step |
| Center X / Center Y | 0–100%, 2.5% per step |

The source wedge begins at the rotation angle; alternating reflected copies fill the other sectors. Mapping beyond the rectangular source folds at its edges. At an extreme center with four segments, some angles can leave most or all of the image unchanged because it already lies in the source quadrant.

Compose in LiveView and use the **normal physical shutter**. Half-press retains the last delivered composition. Release between shots. Allow the camera to finish; single shots only. Bursts, video, remote triggers and long exposures are outside this candidate's scope.

Settings save automatically when idle. The two checked preference slots are private to Kaleido. If a card write fails, the menu reports that settings could not be saved.

## What needs camera acceptance

After one reboot, check LiveView responsiveness and mode recovery, autofocus and Auto ISO, then take representative 4-, 6- and 12-segment photos with changed angles and centers. Verify Canon quick review and Play, and decode the actual CR2 files to inspect framing, colors, detail, unchanged optical black and the expected pattern. Repeat a normal single shot after releasing the shutter.

General radial RAW processing is slower than the axial Mirror Worlds path. This candidate retains the guarded capture handoff and a **two-second post-hoc hold budget**. If the elapsed hold exceeds that budget, processing finishes before Canon resumes, but the effect stops for following shots. This is not a hard processing deadline or watchdog guarantee. No EOS M processing time is claimed from desktop benchmarks.

An early guard rejection leaves that shot unprocessed. A capture fault remains latched until OFF/ON; if it persists, fully restart and retain the actual CR2 and error code for diagnosis. Once sensor writing starts, the complete transform finishes without yielding or cancellation, avoiding a partly modified frame.

To uninstall, power OFF and remove `ML/MODULES/kaleido.mo`; deselect it in Modules if needed. Do not dynamically unload, replace or remove a running module.

## Package

`BUILD.json` and `IMPORTS.json` record the target/import checks. `SHA256SUMS` covers the extracted files. Source and license are included. Camera-loaded and camera-capture-tested flags intentionally remain false.
