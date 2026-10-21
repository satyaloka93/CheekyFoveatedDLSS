# Cheeky NR bridge preview 4 — old/new NR switching

Standalone and UEVR **upgrade packages for existing working installations**.
Includes preview3 fixes plus a restart-required NR DLL selector and independent
NR settings for each version. NVIDIA DLLs, personal INIs, loaders, OpenXR layers,
UEVR backends and the RenoDX addon are **not bundled**.

## Install

Close the game and back up replaced files and settings.

- **Standalone ZIP:** extract beside the game executable (Cyberpunk `bin\x64`),
  merging `CheekyFoveatedDLSS`. Replace Host and Runtime together.
- **UEVR ZIP:** extract into `%APPDATA%\UnrealVRMod\<game-executable-name>`,
  preserving `plugins` and `scripts`. Update an existing global installation
  in its original location instead; do not create a duplicate integration.

Keep your existing loader, OpenXR layer and current `nvngx_dlssnr.dll`.
Place your old library as **`nvngx_dlssnr_old.dll`** beside
`CheekyFoveatedDLSSRuntime.dll` (preferred), or beside the game executable.
For standard UEVR layout this is `<profile>\plugins\CheekyFoveatedDLSS\`.
Do not install the old RenoDX addon alongside Cheeky for this feature.

## Switch in the menu

1. Open Cheeky → **DLSS-NR** (UEVR: LuaLoader → Cheeky Foveated DLSS).
2. Choose **Use previous NR next launch** or **Use current NR next launch**.
3. Fully exit and restart the **game**. Reinjecting UEVR is not sufficient.
4. Confirm the loaded DLL path/version and increasing successful evaluations.

First Previous launch starts with **NR off and default NR tuning**. Enable NR
after restarting; subsequent launches restore that version's saved tuning.
The menu reports this session's profile separately from the next-launch choice.
Until restart, edits affect the active profile, not the pending one.

Standalone menu key: preserve your existing binding. For a missing menu INI,
run `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Setup-CheekyMenu.ps1`
from the extracted package to create an F11 binding without overwriting one.
Without a valid override the host defaults to F8. See `OVERLAY-MENU.txt`.

## Switch in the INI before launching

With the game closed, edit **CheekyRuntime.ini**, preserving other sections:

```ini
[NR]
PreviousRuntime=1
```

| Value | Library | NR profile |
|---|---|---|
| `0` (default) | `nvngx_dlssnr.dll` | `CheekyNR-current.ini` |
| `1` | `nvngx_dlssnr_old.dll` | `CheekyNR-previous.ini` |

INI locations:

- Standalone: `<game executable folder>\CheekyFoveatedDLSS\CheekyRuntime.ini`
- Cyberpunk: `Cyberpunk 2077\bin\x64\CheekyFoveatedDLSS\CheekyRuntime.ini`
- UEVR: `%APPDATA%\UnrealVRMod\<game-executable-name>\CheekyRuntime.ini`

The two NR profiles live in that same **config directory**. Only `Nr*` keys
are restored from them; SR/gaze settings remain shared in the main INI.
Once profiles exist, make manual NR edits in the selected NR profile.

## Compatibility and troubleshooting

Previous targets **310.8.0.0**: one network (Default), with appearance Model
A/B/C still available. Its menu disables newer network A/B and F5 controls
and benchmarks. Current guidance targets **310.9.16.0**. These are filename
slots, not an automatic DLL downloader or arbitrary-version compatibility.
This changes **DLSS-NR**, not DLSS-SR or its presets.

A missing/invalid Previous DLL does not silently fall back to Current.
Check the runtime log for selected paths and load errors. Standalone support:
send `CheekyFoveatedDLSS-Host.log`, `CheekyFoveatedDLSS-Standalone.log` and the
relevant INIs. No fresh host log can mean a loading or folder-permission issue.

Release build, profile/persistence tests, old-filename fake-NGX DX12 processing,
UEVR lifecycle, DX12/Streamline, Vulkan and overlay regressions passed. ZIP
payloads are hash-verified. Cyberpunk user reports the switch working and logs
confirm old310.8 loaded; old-DLL UEVR gameplay validation remains pending.
No performance or visual-quality improvement is promised. See the included
`NR-VERSION-SWITCH.txt` and `START-HERE.txt` for details and rollback.
