---
type: title-profile
title: The Outer Worlds 2
description: Native D3D12 UE5 title run under UEVR; dispatcher fix verified working, stereo geometry and frame cost still open.
tags: [tow2, uevr, vr, d3d12, streamline]
timestamp: 2026-09-17
resource: "steam://run/1449110"
---

# The Outer Worlds 2

## AFW bridge deployment — 2026-09-17

The AFW ReShade-owned bridge build from commit `3c15aa4` is deployed to
`Arkansas/Binaries/Win64`. The addon SHA256 is
`d0c6922ce6bd26e46279a0d7bd816e4306027a9439b1c6a2b712a6cddd2307fb`; the
matching UEVR projection bridge is installed in
`%APPDATA%/UnrealVRMod/TheOuterWorlds2-Win64-Shipping/plugins` with SHA256
`1acabb6b31020586a03b00a1287fe41842d9b4d837b15a78d995c016228aeadd`.
Existing binaries were preserved with `.pre-20260917-afw-bridge.bak` suffixes.
No ReShade or UEVR settings were changed. Headset validation is pending.

`F:\SteamLibrary\steamapps\common\TheOuterWorlds2\Arkansas\Binaries\Win64\`

## Facts

* Native **D3D12**. ReShade loaded as `d3d12.dll`. Two `ReShade.ini` exist — the one that matters
  is in `Binaries\Win64`, not the game root.
* Uses **Streamline** (`sl.interposer.dll`, `sl.dlss_g`, `sl.pcl`, `sl.reflex`, `sl.deepdvc`) for
  FrameGen and Reflex only — **not** for super resolution.
* `bHardwareRayTracing=False`, so DLSS Ray Reconstruction is not in use. An earlier
  `nvngx_dlssd.dll` hook block was added on the opposite premise and reverted.
* `nvngx_dlss.dll` loads and unloads twice, ~268 ms each, and is gone before gameplay. Only
  `_nvngx.dll` is resident when frames are drawn — this is the title that exposed
  [defect 1](../architecture/ngx-dispatcher-vs-runtime.md).
* Add-on sees **6 distinct view ids**.

## Last run — 2026-09-07 16:07–16:11

```text
D3D12 crop prepare seq=0     flags=0x0000004B flagMV=low mv=2112x1320 low=(528,412 1056x528 fit=yes) selected=low
D3D12 crop prepare seq=18000 flags=0x0000004B flagMV=low mv=1272x1296 low=(318,405 636x518 fit=yes) selected=low
DLSS-NR 310.8 feature-18 runtime initialized
DLSS-NR feature 18 created view=2253263198360 input=1280x640 output=1280x640 scale=1.00 flags=0x4B preset=1
DLSS-NR feature 18 created view=2253766962312 input=1280x640 output=1280x640 scale=1.00 flags=0x4B preset=1
DLSS-NR region codec created region=1600x800 working=1280x640 source=800,624
```

* 38 `crop prepare` lines sampled (every 600th), reaching `seq=18000` — crops are being applied,
  not rejected. Defect 1 is fixed here.
* Both DLSS-NR features land at the **same** `region=1600x800@800,624`, confirming the per-eye
  offset is zeroed.
* Create flags `0x4B` = AutoExposure | DepthInverted | IsHDR | MVLowRes.
* Motion vectors are low-res every frame; the high-res candidate never fits.

## Settings on disk (ReShade persisted them at exit, so they are authoritative)

```ini
[CheekyFoveatedDLSS]
Enabled=1        AlignmentBorder=1    D3D11D3D12Transport=0
Width=0.5        Height=0.4           HeightOffset=0.04    XOffset=0
TransitionWidth=0                     Roundness=0
NrEnabled=1      NrFoveated=1         NrUseSrFoveation=1   NrPreset=1
NrWidth=0.5      NrHeight=0.4         NrHeightOffset=0.03  NrXOffset=0
NrTransitionWidth=0.08                NrWorkingScale=0.8   NrIntensity=2
```

`XOffset` was `0.31` until 2026-09-07 and is the origin of the mirrored two-box geometry — see
[Stereo eye roles](../architecture/stereo-eye-roles.md).

## Open

1. Does `XOffset=0` actually produce one fused box in the headset? **Untested by eye.**
2. Frame cost. The private feature is evaluated **per eye**; at TOW2's internal resolution DLSS is
   sub-millisecond, so a 20% region cannot recover the doubled overhead. Expect foveation here to
   be neutral-to-negative regardless of geometry.
3. `D3D11D3D12Transport=0` is a leftover from the System Shock crash hunt. Harmless on a native
   D3D12 title, but it is not a deliberate setting.

## Environment note

Run under UEVR. `VR_AFW_FramewarpMode=0` (AFW off) as of 2026-09-07 — enabling AFW mid-session
produced `DXGI_ERROR_DEVICE_REMOVED` (`0x887A0005`) and a lockup. UEVR's own log lives with the
UEVR backend, **not** in the game folder.
