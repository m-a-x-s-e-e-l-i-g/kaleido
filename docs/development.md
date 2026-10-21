# Development

This is a separate Git project derived from Mirror Worlds 0.2.0. Only the pixel/camera dependencies needed by Kaleido are retained. There is no dependency on a sibling checkout at build or runtime.

- `modules/kaleido/`: menu, persistent controls, namespaced descriptor and guarded capture handoff.
- `modules/fx_common/`: fixed-point UYVY preview, complete 2×2 Bayer-cell mapping, packed Canon14 transformation and display ownership.
- `modules/glock/`: inherited formatting, memory ABI and Canon review helpers, not a glock module.
- `targets/EOSM.202/`: exact public installed-core symbols and provenance.
- `tests/`: real native backend plus synthetic Canon/OS/FIO transports.
- `site/`: local-only photographic playground, no external runtime requests.

All exposed segment counts divide 360 exactly: 4, 6 and 12. Preview, browser and RAW use the same Q14 sector/rounding convention. RAW maps cells rather than individual red/green/blue samples, retaining CFA phase. Nearest-cell sampling is intentional; interpolation across CFA colors is not performed.

## Fast LiveView in rc2

The general preview prepares one 32-bit code per UYVY pair when geometry changes. Each code holds the first source-pair address, both Y selectors, and a local source-pair delta. The hot loop gathers aligned words and averages signed U/V in separate byte lanes; no sector lookup, coordinate rotation or rounding is repeated per frame. Four pairs are processed per loop iteration. Axial copy/reverse paths remain in place. Unaligned input or legacy geometry outside the compact delta range uses the exact previous implementation.

The 720×480 workspace is 740,516 bytes, an increase of 691,300 bytes over rc1. Capture scratch remains 895,796 bytes. Both are subject to the existing allocator and 1.5 MiB reserve. Half-press geometry, Canon controls, buffer ownership, cache publication and RAW handoff are preserved.

The inherited edge tag was 180, overlapping a legal rotation. It now uses a non-angle tag so 180° follows the radial path in both preview and RAW. Preference encoding is unchanged. UI numeric values use `%d` with explicit signed casts because the pinned Canon formatter does not handle `%u` as expected.

For reproducible performance comparisons, install `unicorn==2.1.4` and `pyelftools==0.32` into `.cache/arm-runtime`, then run `python tools/benchmark_preview.py`. The tool compiles rc1 fixtures and the current source with the GNU ARM builder, checks complete output hashes and callee-saved registers, and counts ARM instructions. Host timing and instruction counts do not model the physical EOS M display scheduler, caches, memory bus or FPS.

## Building

`python scripts/fetch_ml_build.py` obtains and checks the pinned official archive. If the official server rejects an automated download, obtain the exact archive from the URL in that script and place it at `.cache/magiclantern.2025-06-20.EOSM.202.zip`; the script still verifies its SHA-256 before use. Never substitute another core build.

`python tools/build_module.py` fetches pinned ML source and uses the Docker builder to produce `build/modules/kaleido/kaleido.mo`. Warnings are fatal. The tool checks exact imports, module identity, property-handler registration and ARM relocation/branch structure. Build metadata has a fixed reproducibility epoch. Successful audit is not hardware evidence.

`python scripts/test_native.py --full` compiles and executes the actual C renderer and packed RAW backend, checks independent bitstream reads, optical margins/padding, cancellation, invalid workspaces, all exposed segment families, moved centers and multiple angles. It also verifies preference recovery and the real capture gate/observer/backend with synthetic DMA and shutter events. Use `--only native`, `--only settings` or `--only gate` for focused follow-ups.

The native suite exports fixed coordinate fixtures to `evidence/geometry.json`. `node --test tests/playground.test.mjs` compares browser geometry against these compiled C results in addition to the independent four-quadrant oracle.

`python scripts/build_playground.py` assembles a self-contained `playground.html`: JS, stylesheet, fonts and the sample illustration are embedded. Opening this file directly needs no HTTP server. The candidate download remains a relative link into the project.

## RAW ownership and timing

Kaleido retains the firmware/caller/DMA/reader guards from Mirror Worlds. The generic transform uses 895,796 bytes for the 4,478,976 active Bayer cells, borrowed through the driver's exclusive capture-scratch lease. All mapping closure and cancellation checks precede the first sensor write. It then peels the cell-dependency graph and rotates residual cycles using one four-sample temporary. No complete sensor snapshot is allocated.

The borrowed preview canvas is invalidated before use as scratch and is not exposed to the display until a new accepted render commits. Its allocation and family claim remain held through stop/shutdown until the lease is returned. Other MAXmade effect modules must be disabled at the Modules level and the camera restarted, because older siblings do not know Kaleido's namespace.

The inherited full-capture hold budget is two seconds, checked after completion; no longer duration is silently approved by this candidate. Desktop runtimes are not EOS M estimates. Physical camera timing and watchdog behavior remain required acceptance work.

## Packaging

Run `python tools/package_module.py --instructions docs/camera-candidate.md`. Packaging rechecks the actual module imports, source/artifact hashes and target table, then writes a reproducible stored ZIP with fixed timestamps, member modes and checksums. It refuses to replace an existing release. `python scripts/verify_candidate.py` checks the payload, all extracted checksums, source/build correspondence and reproducible ZIP bytes.

Do not publish this candidate as a camera-validated stable release before checking actual rebooted camera behavior and decoded CR2s.
