# Cheeky NR bridge preview 3 — Standalone and UEVR

Upgrade packages for existing working installations, not fresh installers.
Close the game and back up replaced files/settings first. Keep your current
loader, OpenXR layer and NVIDIA DLLs; this release does not install or replace
them. Do not install both the UEVR plugin and ReShade addon.

## Choose your package

- **Standalone-Upgrade**: extract beside the game executable (Cyberpunk:
  `bin\x64`), merging `CheekyFoveatedDLSS`. Replace Host and Runtime together.
- **UEVR-Upgrade**: extract into `%APPDATA%\UnrealVRMod\<game-executable-name>`,
  retaining `plugins` and `scripts`. Update an existing global installation
  in its original location instead; do not create a duplicate.

## Standalone menu key

From the extracted standalone package, run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Setup-CheekyMenu.ps1
```

This creates `CheekyFoveatedDLSS\CheekyOverlay.ini` only if missing, with
`[Overlay] MenuKey=122` (**F11**). Existing bindings are preserved.
Alternatively copy `defaults\CheekyOverlay.ini` beside the Host DLL.
Without a valid override the host uses **F8**. With the game focused, press
your configured key. Change it under **General → Change menu key**.
See `OVERLAY-MENU.txt` for manual key values and troubleshooting.
UEVR uses **LuaLoader → Cheeky Foveated DLSS**, not this standalone INI.

If the standalone menu does not open, send `CheekyFoveatedDLSS-Host.log`,
`CheekyFoveatedDLSS-Standalone.log` and `CheekyOverlay.ini` from the
`CheekyFoveatedDLSS` folder. Logging does not require opening the menu.
No fresh host log can mean the DLL never loaded or the folder is unwritable;
include your loader log in that case.

## Changes since preview 2

- Cyberpunk VR asymmetric camera and frame/viewport routing compatibility.
- Clearer shared NR intensity/blend and exposure/reference-white organization.
  Shared strengths apply equally within each enabled pass; network/appearance
  and filter choices remain per-pass.
- Optional peripheral DLSS SR and experimental F5 internal-feature reuse,
  with the F5 control also exposed in UEVR. See `NR-F5-REUSE.txt`.
- Scene resets reach all independent NR passes; F5 toggles reset history.
- Standalone menu setup and automatic input/graphics initialization diagnostics.
- Manual screenshot hotkey has no intentional countdown; menu capture retains
  its countdown. Capture requires the separately configured SteamVR helper.
- Motion-reprojected correction reuse and its benchmark are **not included**:
  NPC-face flicker was reported. This is distinct from retained F5 reuse.

Existing settings are retained; upgrading does not enable NR or extra passes.
Read `START-HERE.txt` for tuning, benchmark limits and rollback.

## Validation and limits

Release build, NR processing, UEVR lifecycle/settings, standalone DX12 and
Streamline processing, Vulkan regression, and DX11/DX12 overlay tests passed.
Packaging checks verify file hashes, missing-host rejection, F11 INI creation
and preservation of existing bindings.

This remains a prerelease. Automated tests do not establish headset quality,
real-model parity or a performance gain. Latest UEVR headset validation is
pending. No proprietary NVIDIA DLLs or capture executable are redistributed.
