Cheeky NR bridge preview 5 — Cyberpunk SBS gaze and ViT cadence
============================================================

Fixes eye tracking with Cyberpunk VR Port 0.1.9 SBS rendering: valid headset
gaze was rejected because the two native DLSS views lacked camera projections.
Cheeky now recognizes the SBS eye viewports and binds each native evaluation
to the port's published eye projection. The user reports working tracking;
runtime logs confirm both projections, accepted fresh gaze and moving crops.

Standalone and UEVR menus now expose VitEvery (FP8) and VitEveryPlain (FP16).
These control the experimental F5 reuse implementation, not the NR network
model or precision. 1 computes every frame; 2 allows alternate-frame reuse
when the F5 master switch is enabled. Missing keys default to 1/1, including
existing profiles. Both 1 bypass reuse and its cache copies. The F5 benchmark
temporarily uses 2/2 for its OFF/ON comparison and restores your settings.
Cache metadata is bounded; this is NOT a confirmed fix for the reported
memory growth. Restart the game to release previously retained GPU buffers.
F5 reuse remains limited to native DX12 with NR 310.9.16.0.

## Installation

These are **upgrade packages for existing working Cheeky installations**.
Close the game and back up the files you replace.

- **Standalone-Upgrade:** merge `CheekyFoveatedDLSS/` beside the game executable
  (Cyberpunk: `bin/x64`). Keep your existing loader, INIs and NVIDIA DLLs.
- **UEVR-Upgrade:** merge `plugins/` and `scripts/` into the game's profile
  under `%APPDATA%/UnrealVRMod/<game-profile>/`.
- No NVIDIA libraries, personal settings, port DLLs or OpenXR layer installer
  are included. Keep your working layer setup. No registry change is required.
- Cyberpunk SBS: select **Shared right-eye gaze (Cyberpunk compatibility)**,
  or set `CenterMode=3` under `[CheekyFoveatedDLSS]` in the main Cheeky INI.
  The recovery path requires the port's default per-eye SBS DLSS mode and
  eye windows enabled.
- Standalone menu: run `Setup-CheekyMenu.ps1` beside the game executable if
  you need the default F11 binding. It preserves an existing overlay INI.
  To choose a key, edit `[Overlay] MenuKey` in
  `CheekyFoveatedDLSS/CheekyOverlay.ini` (122=F11, 121=F10), then restart.
  See `OVERLAY-MENU.txt` for controller bindings and failure logs.
- For INI cadence edits, use `VitEvery=1` and `VitEveryPlain=1` under
  `[CheekyFoveatedDLSS]` in the active `CheekyNR-current.ini` profile when
  present; otherwise use the main INI. The menus save the correct profile.
- Current/Previous NR switching remains restart-required and keeps independent
  tuning profiles; see `NR-VERSION-SWITCH.txt` in both downloads.

## Validation and limits

Release build, shared-gaze regression, NR processing, UEVR lifecycle/settings,
standalone native DX12 and Streamline, Vulkan and overlay UI tests passed.
Cyberpunk SBS received a gameplay test; the new UEVR cadence controls received
automated tests, not a new headset test. No measured performance or image-quality
gain is claimed. Restore backed-up binaries to roll back.

Credits: ClarkCheekyKent, Chtiblue, janblade/OptiScaler contributors and this
fork's compatibility work. GPLv3 source is available at the release tag.
