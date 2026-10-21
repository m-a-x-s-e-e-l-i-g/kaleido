# Kaleido 0.1.0-rc2 validation

Validation date: 2026-10-21. rc1 camera feedback reported literal unsigned format specifiers and low LiveView FPS. The rc2 measurements below are host/ARM component results, not a physical-camera FPS measurement.

## Menu regression

The actual menu frontend was compiled with a Canon-compatible signed-decimal/string formatter. All 360 rotation labels, 82 center labels, wrapping/bounds, segment help, numeric capture error, preference persistence and restart checks passed. Examples now render `17 deg`, `50.0%`, and `52.5%`.

## Preview performance and exact pixels

The archived rc1 source and current source were compiled using the GNU ARM builder at ARMv5TE/O2 with long calls. Unicorn executed the real component code, preserved callee-saved registers, and produced full 720x480 UYVY frames matching the host and prior release byte-for-byte. Each measured full-resolution frame includes every Y/U/V byte; the image is not reduced or simplified.

| Geometry | rc1 ARM instructions/frame | rc2 ARM instructions/frame | Reduction |
| --- | ---: | ---: | ---: |
| six-default | 29,945,980 | 3,795,985 | 7.89x |
| six-rotated | 30,453,803 | 4,519,185 | 6.74x |
| twelve-rotated | 30,466,942 | 4,317,229 | 7.06x |
| four-axial | 1,681,378 | 1,681,873 | 1.00x |
| four-shifted | 30,773,171 | 4,830,193 | 6.37x |

The already-fast four-way axial path remains effectively unchanged; its extra dispatch check is below 0.1 percent instruction overhead. The compact lookup adds 691,300 bytes of private workspace and is prepared only when geometry changes. The existing scratch lease and 1.5 MiB allocator reserve remain in force.

See [baseline](../evidence/rc2/baseline-performance.json), [current](../evidence/rc2/performance.json), and [comparison](../evidence/rc2/comparison.json) for input hashes, one-time preparation work, host timings and frame hashes. Instructions do not model the EOS M cache, memory bus, display scheduler or actual FPS. The rc2 FPS/camera acceptance flag is intentionally false.

## Native checks

`python scripts/test_native.py --full` passed with 155,857,624 native pixel checks and zero failures, plus preference, capture gate, UI and parity checks. The [complete log](../evidence/rc2/native.log) records:

- 63 preview/RAW settings combinations, including 180 degrees and opposite extreme centers.
- Three complete packed 5280x3528 Canon14 buffers, preserving the 5184x3456 active region mapping, margins, padding and Bayer phase.
- Real gate/observer/backend with synthetic Canon DMA transport: repeated shots, latched controls, review priority, caller/scratch/cancel/shutdown/reader rejection, and cancellation during planning.
- 349 complete UYVY comparisons against rc1, including full-size, padded, unaligned and tiny/tail cases. AddressSanitizer and UndefinedBehaviorSanitizer reported no errors; the test disables ASan's recursive SIGSEGV handler in this Docker environment while retaining address instrumentation.
- All menu value, help, bounds, restart and persistence checks.

The old internal edge tag was 180. A non-angle tag now permits the exposed 180-degree rotation in both preview and RAW. All three counts at 180 degrees also pass ARM-vs-host frame comparisons. Browser geometry tests include this angle and match 1,800 compiled C point fixtures.

## Build and package

The linked module passed the exact pinned-core import, property-handler and ARM relocation audit with 58 imports. Two final module builds were byte-identical; see [build evidence](../evidence/rc2/build.txt). The package verifier checks every archive member, source/module hashes and byte-identical ZIP reconstruction. The original rc1 ZIP remains unchanged. Package checksums are retained in `releases/SHA256SUMS`.

## Remaining camera acceptance

No rc2 camera FPS or decoded CR2 result is claimed. Install only `kaleido.mo` while powered off, restart, then verify labels, responsiveness, AF/Auto ISO, capture timing and Canon review/Play on the original EOS M. The existing two-second post-hoc RAW capture budget is preserved.
