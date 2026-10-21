Kaleido
=======

:Author: Max Seelig (MAXmade)
:License: GPL-2.0-or-later
:Summary: Radial kaleidoscope for LiveView and native Canon RAW
:Version: 0.1.0

Compatibility
-------------

For original EOS M / Canon 2.0.2 /
Magic Lantern dev 2025-06-20 (ed3e7c0). Author camera-tested rc4;
0.1.0 retains that implementation with updated version metadata.

Controls
--------

Enable kaleido.mo in Modules, restart, open Shoot / Kaleido.
Use native 1x LCD / Global Draw ON / A+ / Auto ISO /
Canon RAW / single-shot drive. Enable Live effect.
Choose 4, 6 or 12 segments, rotation and center position.
Compose and use the physical shutter; release between shots.
Settings survive restart. Effect starts OFF on first install.

Capture
-------

The capture path preserves whole Bayer cells and optical black.
General rotated RAW capture uses a checked in-place dependency
transform with private scratch; processing can be slower than
Mirror Worlds. Allow processing to finish between single shots.
The author reports the preceding rc4 works well on the camera.

Installation
------------

Disable all other MAXmade effects before loading this module.
Copy only ML/MODULES/kaleido.mo while camera is powered OFF.
Keep existing autoexec.bin and firmware. No dynamic unload.
