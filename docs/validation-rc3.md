# Kaleido 0.1.0-rc3 validation

Date: 2026-10-21. rc2 camera feedback still reported slow LiveView and long post-shot processing. rc3 targets memory locality, the unnecessary native preview copy, and the RAW dependency algorithm. All measurements below are components on a host/emulated ARM, not measured EOS M FPS or capture time.

## LiveView

The original full 720×480 UYVY output remains byte-identical. Lookup rendering now visits 16-pair × 16-row tiles, reusing neighboring rotated source pixels. Native spatial capture retains delivered controls rather than copying another 691,200-byte frame into unused history. The existing cache publication call remains.

ARM execution checks the complete frame hash and R4–R11 preservation. Compared with rc2 rendering plus its actual `fx_copy_cpu` canvas commit, instruction work is unchanged or slightly lower (1.00–1.05× reduction). The main improvement is memory locality:

| Geometry | 8 KiB model: all misses reduced | 16 KiB model | 32 KiB model |
| --- | ---: | ---: | ---: |
| Six, default | 3.30× | 3.02× | 1.67× |
| Six, rotated/moved | 3.57× | 2.94× | 1.73× |
| Twelve, rotated | 2.61× | 2.04× | 1.66× |
| Four, axial | 2.32× | 2.32× | 2.33× |
| Four, rotated | 2.57× | 1.91× | 1.58× |

These are cold, four-way LRU, 32-byte-line, write-allocate models at fixed synthetic buffer addresses. They exclude actual bus/scheduler timing and are **not physical cache measurements or FPS multipliers**. The pinned ARMv5 cache maintenance assumes 8 KiB; larger models are included to show sensitivity. See [preview evidence](../evidence/rc3/preview-performance.json). A comment-only source clarification was rebuilt to an identical measured ARM ELF, recorded in that report.

## RAW processing

Referenced source cells that map to themselves stay untouched. Only referenced non-fixed cells are snapshotted, with a bitmap/rank table locating their four samples. Output then uses exact Q14 constant-sector scans and packed 28-bit Bayer pairs. Centered four-way 0/90-degree geometry dispatches to the grouped Canon14 axial implementation. If sparse snapshots cannot fit, the graph fallback also uses sector scans and avoids repeated software pixel-ID division.

| Reduced packed sensor fixture | rc2 ARM instructions | rc3 ARM instructions | Reduction |
| --- | ---: | ---: | ---: |
| Six, default | 33,996,516 | 6,798,784 | 5.00× |
| Six, 17°, center 42.5/57.5% | 37,674,503 | 7,120,935 | 5.29× |
| Twelve, 179° | 39,919,354 | 8,162,203 | 4.89× |
| Four, axial | 32,933,840 | 345,930 | 95.20× |
| Four, 17° | 32,904,136 | 6,657,693 | 4.94× |
| Six, 359°, extreme center | 67,282,573 | 44,732,500 | 1.50× |

The ARM fixture is 320×212 with a 304×204 active area and proportionally scaled scratch; it is **not a full-sensor processing-time estimate**. Every resulting byte matches host execution and rc2, with callee-saved registers intact. [ARM evidence](../evidence/rc3/sensor-arm.json).

Five complete 5280×3528 Canon14 buffers, including the 5184×3456 active area, also match rc2 byte-for-byte. Three host timings per case show median radial reductions of 2.49–5.39× and 33.03× for axial; the machine was not dedicated, samples vary, and these are not EOS M estimates. No CR2 compression, SD transfer or camera watchdog is modeled. [Full-sensor evidence](../evidence/rc3/sensor-host.json).

The preferred private capture arena is 2 MiB. Low-memory and preferred-allocation failure tests recover to the previous 895,796-byte minimum while preserving the 1.5 MiB reserve. Difficult geometry can still require the slower graph fallback. No full sensor snapshot or photo/SRM heap fallback is added.

## Checks and build

- 155,857,624 native checks, zero failures, including complete packed sensors, independent bitstream oracle, optical margins, row padding, CFA phases, moved centers, 180° and cancellation.
- The same native suite passes AddressSanitizer and UndefinedBehaviorSanitizer.
- 349 complete UYVY parity configurations pass, including unaligned inputs, padded/tiny frames, lookup tails and legacy fallback.
- Actual camera driver tests cover no per-frame scratch copy, half-lock/release, rejected-frame fallback using Canon input, lease exclusion, stop, reserve and allocation recovery, and Canon review/Play.
- Actual capture gate/backend passes repeat, caller rejection, scratch failure, cancellation, shutdown, competing readers and planning cancellation, using synthetic Canon transport. Source bytes stay unchanged on rejection, registers and scratch leases are preserved, and no files are written from hooks.
- Numeric menu values, settings recovery/persistence and all six playground tests pass.
- Final module audit passes all 58 exact-core imports, registered property handlers and ARM relocations. An independent manifest-verified source tree with a fresh pinned SDK checkout rebuilds byte-identical `kaleido.mo`.

Module SHA-256: `141612a71b088fbc7c89b01db688402af3565807d7d7fab27d647bcc56906477`.

The [evidence folder](../evidence/rc3/) records these checks. The immutable rc1/rc2 ZIPs remain intact. The candidate verifier checks every member/source/module checksum, import audit and byte-identical ZIP reconstruction.

One final PIE sanitizer process exited 139 before producing test output; the unchanged binary passed with ASAN startup tracing enabled. The host parity target now uses `-fno-pie -no-pie`, with both sanitizers retained, and passed three consecutive executions. The original failure, traced rerun and final passes are retained in the evidence folder. This affects the Docker host test layout, not the ARM module.

## Camera acceptance

This candidate has not been run on the physical EOS M by the implementation tools. Actual LiveView FPS, post-shot time, AF/Auto ISO, mode recovery, Canon review and decoded CR2 output remain unverified. The existing guarded handoff, 100 ms observer hold and two-second post-hoc capture budget are preserved. No higher budget or physical speed guarantee is substituted for the optimizations.
