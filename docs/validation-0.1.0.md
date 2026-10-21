# Kaleido 0.1.0 verification · 2026-10-21

## Physical camera report and release scope

Max Seelig reports that **0.1.0-rc4** was tested on the original EOS M and “everything works great”. The supplied screen captures are real camera output. This is author-reported hardware testing; no measured FPS, CR2 decode report or per-feature test matrix was supplied. The exact predecessor archive and module hashes are recorded in [camera-test-report.json](camera-test-report.json).

The stable build checks every implementation source against that rc4 manifest. Only `FX_VERSION` in `effect_config.h` and `README.rst` module metadata/documentation may differ. The underlying LiveView, RAW, memory, menu controls and capture algorithms are unchanged. New screenshots, documentation, packaging metadata and website content accompany the release.

The build tool's `camera_loaded` and `camera_capture_tested` flags remain false for the newly versioned binary. Its separate `camera_test_report` and `same_implementation_as_camera_tested_predecessor` fields describe the reported rc4 test and verified source relationship. Automated checks are not presented as physical-camera observations.

## Automated checks

- ARMv5TE module build against pinned Magic Lantern `ed3e7c0dfacfdeaf3349dee73edf9e3621217462`, with compiler warnings fatal.
- Exact EOSM.202 core: **58 imports**, module identity, property registration and relocation audit pass.
- Full native suite: **155,857,624 checks; zero failures**, including three full 5280 × 3528 packed Canon14 fixtures and optical margins.
- Seven guarded capture transport scenarios, preference recovery, menu formatting and persistence pass.
- Full UYVY preview parity: **349 configurations**, including 250 lookup preparations and 75 exact legacy fallbacks, under AddressSanitizer and UndefinedBehaviorSanitizer.
- Shared-arena camera lifecycle fixture covers low-memory four-segment startup, 6/4/12 changes without allocations, RAW scratch exclusion, map rebuilding, half lock, stop and Canon review. Its separate sanitizer run uses the real camera driver with synthetic transport.
- Browser geometry: **6 tests pass**, including compiled-C coordinate parity and an independent four-way reflection oracle.
- Freshly staged manifest-verified sources, with a clean pinned SDK checkout and no predecessor ZIP, rebuild **byte-identical `kaleido.mo`**. Module SHA-256: `144139a4821bfad0f1344583bfb5bcf881b9995485d6c2efbe80f51b5a83617a`.
- Package verification checks every member and checksum, source/module correspondence, exact imports, absence of replacement firmware/core, and byte-identical ZIP reproduction. `releases/SHA256SUMS` records the archive hash.

Recorded native, sanitizer, build and independent rebuild logs are under `evidence/validation-0.1.0-*`.

## Screenshot and site checks

The three new 720 × 480 images are lossless WebP conversions with exact RGB pixel equality to VRAM2/3/4.BMP. Source hashes and conversion results are in [the conversion evidence](../evidence/camera-screenshots-0.1.0.json). All five camera captures appear in the README, website and self-contained playground.

Desktop and mobile browser checks verify loaded images, an expandable Magic Lantern FAQ, no horizontal overflow, real MAXmade hyperlinks, and Mirror Worlds as the final recommendation above the footer. The site and release download are assembled from the immutable public ZIP.
