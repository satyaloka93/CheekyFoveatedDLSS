---
type: fix
title: AFW single visible foveation region and beta.4 identity
description: Consolidate raw and filtered stereo donor masks for compositing and recognize the verified beta.4 metadata prefix.
timestamp: 2026-09-19
tags: [afw, foveation, diagnostics, unresolved]
---

The TOW2 user reported three circles and worse performance when enabling SR.
AFW gaze builds up to four donor masks (raw/filtered for each eye). These were
passed directly to rounded compositing, exposing multiple overlapping lobes.
The compositor now receives one bounding ellipse via `afw_single_region`,
while donor crop allocation and projection-space mapping remain unchanged.
The resulting extent can be wider than the user's nominal region because it
must still cover the selected stereo donors. In-game fusion remains to verify.

## Failed visual validation and follow-up

The user subsequently reported three **ovals** in TOW2, unchanged ghosting,
and three more circular outlines in SHf. The bounding-ellipse change did not
resolve the visual defect. At that test TOW2 had addon hash `aa83c815...`,
while SHf still had the older `17aad8dc...`; do not attribute both results to
the same binary. TOW2's peripheral scale was later changed by the user to
0.63; preserve it.

Source tracing establishes that `d3d12_composite_shader.hpp` writes the red
alignment border into `GameOutput` before AFW. UEVR crops this color into its
AFW input, then submits the current backbuffer and the other eye's AFW output.
A border embedded here is scene content to the warp, not a final per-eye
overlay. Reprojection/history can duplicate or distort it. This identifies a
duplication path, not proof of the cause of every reported circle or of scene
ghosting. `OutEyeFrameBuffer` is a runtime-owned framebuffer returned by
`InitFrameWarp`; painting into it in place risks contaminating reused images.
The acquired OpenXR destination after its copy, before execution/release, is
a separate insertion point. The follow-up renderer below now uses it.

Follow-up source corrections finalize fixed-position masks too (fixed SR
bypasses the gaze-preview path), and prevent fixed NR from reviving a stale
gaze mask for the same view id. Crop allocation still uses the complete donor
extent before consolidating its visible mask. Regression coverage includes
gaze-to-fixed transitions and fixed SR/NR masks. These changes do **not**
promise a circular final region or repair AFW's temporal ghosting.

Release build, `--afw-core`, `--d3d12-history`, and `--d3d12-composite` passed
after this follow-up. The full suite's previous stall remains separate and
unresolved.

## Final-eye SR border candidate

Implemented `CheekyAFWBorderV1` export and the matching UEVR-AFW-PUBLISH
OpenXR consumer. The host advertises a live, matching-size final-eye sink.
With verified source-eye projection and one mask, successful private SR
evaluation publishes both eye-projected outlines and suppresses its baked-in
red border. The host draws the contour into the acquired eye image **after**
the color copy, before command execution and release. The copy restores the
destination to RENDER_TARGET; its existing RTV and command context are reused.
The AFW input, persistent output, and DLSS history images are not painted.
The addon module is referenced during each export call, not cached across
unload. Stale border data (>100 ms), unknown eyes, projection-generation
changes, incompatible sizes, and absent host support are rejected.

This is an SR diagnostic-outline correction, not a replacement frame-warp
algorithm. The projected donor envelope may still be oval. It is not a
depth-aware proof of the final sharp-region boundary; scene ghosting remains
unresolved. Independent NR borders and OpenVR do not use this new path.
No game/profile/NR/peripheral-scale settings are changed.

Release builds passed; `--afw-core` includes handshake, stale data, both-eye
projection, disabled-border and size mismatch tests. `--d3d12-history` and
`--d3d12-composite` passed. Standalone host contour tests check clipping,
unpainted center, invalid floats and inverted bounds. These do not constitute
an in-headset or final-target GPU performance validation.

Candidate SHA-256:

- Addon: `42a81b9aa56d37d19c79e47111eac00b408c62b3ad31e37a51ebf03b91271500`
- UEVRBackend: `d370eb1649edb87d83ddaef503ba351b2c55dc04bbdedf8274f3e1218e397368`
- PDB: `6bf740e74f09557f48da57a2df10cbfb5d1ac88abce307a00d79c7d9efac0cc4`

Initial deployment aborted before copying because a game was running. After
the user closed it, deployed and hash-verified the matching addons in TOW2
and SHf and the backend/PDB in `C:\Users\gthom\Downloads\UEVR-AFW-JOEY`.
All four prior files have adjacent `.pre-final-border-20260919-183537.bak`
backups. Source hashes were checked against the tested checkpoint before
copying. No settings or PDAFW runtime changed. In-game validation is pending.
Successful host rendering logs `[Cheeky AFW border] final
OpenXR image`; absence of this line means final-eye routing is not confirmed.

The installed official beta.4 DLL SHA-256
`76bbc4d7a5370ba0d6a1a50de6b81d45223e42b764d2d7c79f0c6c48a59f6b64`
was previously unrecognized. Its decoded prefix matches beta.6: mode offset 60,
eye offset 64, texture size 56/resource offset 8. Only this exact additional
hash/size is allowed; unknown binaries retain opaque observation. This enables
the existing resource-copy-based source-eye identification on beta.4.

TOW2's saved PeripheralDlaaScale was 1.0; runtime logged equal render/working
dimensions plus extra center evaluations. Set it to 0.75 with a config backup,
reducing requested peripheral pixel count to 56.25% before other overhead.
This is not a measured FPS gain. No other game deployment or gaze calibration
is changed. Tests assert one composite mask and matching viewing-ray bounds
between source eyes; the runtime-file test accepts the installed beta.4 DLL.
