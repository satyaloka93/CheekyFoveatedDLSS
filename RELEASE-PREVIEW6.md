# Cheeky NR bridge preview 6 — NR memory cleanup test

This build releases NR feature handles and intermediate textures when NR is
switched off, and retires disabled second/third-pass resources. Release waits
for recorded command lists and GPU submissions to finish. Re-enabling NR
recreates the resources. New counters distinguish NR textures/features from
experimental F5/ViT cache buffers.

**This fixes retained NR resources; it is not a confirmed fix for all reported
VRAM growth while F5 reuse is enabled.** In the earlier Cyberpunk test, turning
NR off released 293.25 MiB of tracked textures and all four NR feature handles.
ViT cache buffers remained at 2.25 MiB in that run. NVIDIA/driver internal
allocations and total process VRAM are not covered by those counters.

## Choose a download

- **Standalone-Upgrade:** for an existing working standalone Cheeky installation,
  including Cyberpunk VR Port. Close the game, back up the existing
  `CheekyFoveatedDLSS` directory, then merge the download's `CheekyFoveatedDLSS/`
  beside the game executable (Cyberpunk: `bin/x64`). Replace Host and Runtime
  together. Keep your existing loader, INIs and NVIDIA libraries.
- **UEVR-Upgrade:** close the game, back up its UEVR profile, then merge `plugins/`
  and `scripts/` into `%APPDATA%/UnrealVRMod/<game-profile>/`. Replace the existing
  plugin in its original location; do not install a second copy.

These are upgrade packages, not complete installers. No NVIDIA DLLs, OpenXR
layer installer, personal tuning INI, port DLL or registry changes are included.
Both packages retain preview 5's old/new NR switch and independent tuning
profiles; switching the NR DLL requires restarting the game.

Standalone menu: keep your existing key. To create a missing overlay INI, run
`Setup-CheekyMenu.ps1` beside the game executable; it defaults to F11 and preserves
an existing file. Edit `[Overlay] MenuKey=122` in
`CheekyFoveatedDLSS/CheekyOverlay.ini` to choose F11 (restart after editing).
See `OVERLAY-MENU.txt` for controller controls and startup failure logs.

## How to test memory cleanup

1. Restart the game for a clean baseline. Use your normal NR settings and note
   process VRAM plus the standalone NR performance page's retained NR texture,
   active/cached feature and ViT buffer counts.
2. Run the demanding workload that previously caused growth. Record whether F5
   reuse is on and the `VitEvery` / `VitEveryPlain` values. F5 reuse is eligible
   only on native DX12 with NR 310.9.16.0; 1/1 bypasses reuse even if enabled.
3. Turn **NR off**, leaving the game running and presenting frames. Once GPU
   work drains, NR texture and feature counters should fall to zero. Pending
   or replayable command lists can delay retirement; this build does not free
   resources still in use by the GPU.
4. Turn NR back on: the effect should return. Repeat off/on and compare the
   counters. Report sustained growth separately from memory that stays cached.
5. Share the standalone log in `CheekyFoveatedDLSS/`, or the runtime log beside
   the UEVR runtime DLL. Include the `NR memory` records, NR DLL version, pass
   count, before/after-upscale mode, F5 cadence and approximate test duration.

Logs record memory counters every five seconds. Standalone also displays them
in its NR performance page; UEVR exposes the runtime diagnostics and log.
ViT buffers remain retained until game exit, bounded by the existing 256 MiB
allocation cap. Turning off NR is not expected to return all process VRAM to
its launch value. Restart releases remaining process-owned buffers.

## Scope and rollback

This is the preview 5 / Chtiblue-based fork plus resource cleanup and diagnostics.
It is **not** the separately tested upstream 0.6.3 adaptation. It does not include
the experimental grain filter, or the later 0.6.3 Fixed-mode alignment changes.
Cyberpunk SBS eye tracking retains preview 5's shared-gaze support (CenterMode=3).

Release build and regression results are recorded in the release description.
The prior Cyberpunk gameplay result applies to the same cleanup implementation;
a new headset validation of these release ZIPs has not been performed.
To roll back, close the game and restore the backed-up binaries/profile.

Credits: ClarkCheekyKent, Chtiblue, janblade/OptiScaler contributors and this fork.
GPLv3 source is available at this release tag. Each ZIP includes SHA256SUMS.txt.
