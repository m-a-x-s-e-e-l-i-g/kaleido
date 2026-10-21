# Kaleido 0.1.0-rc4 validation

Date: 2026-10-21. Camera feedback reported “not enough memory” and no LiveView after changing the segment count to four.

## Reproduction and fix

Changing segments updates the geometry without allocating; memory errors originate in startup. rc3 allocated capture scratch and a separate 740,516-byte preview workspace, and the frontend latched a failed start until OFF/ON. Its optional larger arena could also succeed before the separate workspace allocation failed.

The real camera-driver fixture reproduces the error with four segments and 2,472,756 free bytes: rc3 reports error 10, requires 3,209,176 bytes, and does not run. The same fixture with rc4 runs, reports error 0, and requires 2,468,660 bytes. This reproduces the reported message under controlled memory pressure; the exact physical-camera heap state has not been read.

Kaleido now opts into one arena shared by the preview lookup and RAW scratch. Their users are mutually exclusive under the existing scratch lease. The private allocation decreases by 740,516 bytes at both capacity levels:

| Plan | rc3 owned bytes | rc4 owned bytes | rc4 with unchanged 1.5 MiB reserve |
| --- | ---: | ---: | ---: |
| Preferred | 2,837,668 | 2,097,152 | 3,670,016 |
| Minimum fallback | 1,636,312 | 895,796 | 2,468,660 |

These are private allocator budgets, not total camera RAM or guarantees against fragmentation. No reserve is reduced and no photo/SRM heap fallback is added. Other effects retain separate buffers unless explicitly opted in.

On scratch acquisition, delivered state and the shared map are invalidated. Neither a stale filter nor the worker may interpret or replace capture scratch. After release, the worker rebuilds the map before accepting a frame. Stop retains a leased arena and frees it exactly once after release. Rebuilding after a photo is the memory tradeoff; the rc3 tiled preview and full-precision RAW algorithms are unchanged.

An explicit settings change permits one retry of a failed memory startup after the menu closes. A healthy segment change never restarts or reallocates the effect. Repeated unattended retries and retries of capture faults remain blocked. Memory help includes startup free/required amounts.

## Validation

- The formerly failing four-segment memory-budget fixture passes.
- Repeated 6 → 4 → 12 changes work with every further allocation forced to fail: allocation count stays constant, frames are delivered and settings match.
- Actual packed Canon14 processing borrows the real preview arena; stale callbacks are blocked, preparation is excluded while leased, and the overwritten lookup is rebuilt after release.
- Preferred/minimum allocation recovery, true insufficient memory, half-press lock, rejected frames, stop during a lease, Canon review and unchanged non-sharing behavior pass.
- The camera lifecycle fixture also passes AddressSanitizer and UndefinedBehaviorSanitizer.
- Frontend checks verify one-shot retry after memory failure/menu close, no retry loop, preserved capture faults, unchanged menu formatting and preference persistence.
- The full suite passes 155,857,624 native checks with zero failures, three complete packed sensors, seven guarded capture scenarios, 349 UYVY parity configurations and UI/settings checks.
- All six playground checks pass. Exact pinned-core audit passes 58 imports, registered property handlers and ARM relocations. A fresh pinned SDK checkout rebuilds the manifest-verified source to an identical module.

See [rc4 evidence](../evidence/rc4/) for the original failing fixture, final checks and build logs. Earlier pixel/performance measurements remain in [rc3 validation](validation-rc3.md); no new physical timing claim is made from them.

Module SHA-256: `dc3ccb4e7ff57ba1d86cfb7aedf6a45e723effbcd8ca44236da5fff918ffe21c`.

The new package is immutable and includes source. Previous candidate ZIPs remain intact. Physical camera startup, segment changes, post-shot recovery, AF/Auto ISO, Canon review and decoded CR2 acceptance remain to be verified on the EOS M.
