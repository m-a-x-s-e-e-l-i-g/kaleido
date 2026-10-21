# Kaleido 0.1.0-rc1 verification

Validation date: 2026-10-21. These are implementation/host checks, not EOS M camera results.

## Native pixels and capture transport

`python scripts/test_native.py --full` passed with **149,560,153 checks and zero failures**. The recorded output is [evidence/native.log](../evidence/native.log); the final capture-guard follow-up is [evidence/gate.log](../evidence/gate.log).

- 54 small RAW/preview configurations: 4/6/12 segments, 0/17/90/179/270/359 degrees, centered and opposite extreme centers.
- Three complete 5280×3528 packed Canon14 buffers with the 5184×3456 active region, one per segment count. Each active mapping uses 4,478,976 Bayer cells and 895,796 scratch bytes. Every decoded pixel matched the immutable-source oracle; margins, row padding and CFA parity were preserved.
- Actual UYVY preview compared to the separately implemented RAW mapper at matching cell geometry, including extreme centers. Source and destination guards checked.
- Pre-write cancellation and undersized workspaces leave sensor bytes unchanged.
- Preference round-trip, torn newest slot fallback, recovery and invalid settings passed.
- Real capture gate + eight safety observers + native backend on synthetic Canon/OS/DMA transport passed: three successive shots with 4/6/12 sectors, latched geometry, physical-release rearm, Canon-review priority, caller rejection, scratch failure, cancellation, shutdown and competing-reader rejection. A follow-up also verifies OFF/cancellation during dry planning leaves every sensor byte unchanged and releases the scratch lease. Registers and scratch lease were preserved; no diagnostic file writes occurred.

Desktop runtimes vary with host load and are not camera estimates. The two-second post-hoc camera hold budget remains uncalibrated for this heavier radial path.

## Browser

`node --test tests/playground.test.mjs` passed all six tests, including 1,200 exact browser-coordinate comparisons against compiled C fixtures and an independent centered four-quadrant oracle.

Interactive IAB checks covered 12 segments, rotation, horizontal center, original/effect comparison, reset, local PNG import and PNG export. Desktop and 390px mobile layouts were visually checked; controls remained reachable and the mobile document did not overflow horizontally. Console inspection found no errors or warnings during that check. [The screenshot](playground.png) shows the browser illustration, not camera output.

## Build and package

The final ARM module passed the builder's exact pinned-core import/identity/property-handler/relocation audit with **58 imports**. Two final builds produced byte-identical modules, and the final module has no defined-symbol collisions with Mirror Worlds 0.2.0. See [evidence/build.txt](../evidence/build.txt). Packaging verifies its source and artifact hashes. `scripts/verify_candidate.py` independently audits the packaged module, checks every payload checksum, rejects duplicate paths and replacement core/firmware, and rebuilds byte-identical ZIP bytes from the extracted tree. The package checksum is retained in `releases/SHA256SUMS`.

The standalone HTML embeds its JavaScript, fonts and sample image; syntax and absence of external runtime imports were checked. Direct `file://` execution could not be verified in the in-app browser, which permits HTTP(S) only. The source web playground was tested interactively over localhost.

## Remaining acceptance

No physical EOS M was booted or photographed with as part of this work. Module loading, real preview responsiveness, full-image capture duration, mode recovery, AF/Auto ISO, decoded CR2 quality and Canon quick review/Play remain required. The two build-manifest camera flags intentionally remain false.
