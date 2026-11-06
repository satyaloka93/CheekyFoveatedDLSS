# NR bridge preview 8 — Lecram controls and Cyberpunk test upgrade

Adds optional Lecram-inspired exposure metering, near-black lift protection,
bright-edge darkening protection and temporal detail stability. Both standalone
and UEVR menus expose these controls. Defaults are off/zero. They require the
D3D12 bridge color pipeline; Legacy and Vulkan bypass them.

Temporal stability evaluates NR every frame and filters its correction using
separate eye/pass histories, motion/depth rejection and neighborhood clamping.
It adds GPU work and history memory, capped at 384 MiB of texture allocations.
Try 0 versus 0.3; watch moving faces and head turns for softness or trails.

Also includes Legacy-versus-normalized automatic raw captures, Cyberpunk per-eye
camera handling for coordinated fixed/gaze modes, and optional port vector-audit
hooks used during investigation. Raw pipeline captures bypass temporal filtering.

## Download and install

`Cheeky-Cyberpunk-RC5-Test-20261105.zip` is an **upgrade for an existing working
Cheeky standalone installation**, not a first-time installation package.
Close Cyberpunk, extract the ZIP, and run `Install-Cyberpunk.ps1`, or back up and
replace only the host/runtime DLLs in `bin/x64/CheekyFoveatedDLSS`.
The installer verifies hashes, backs up both DLLs, and preserves loaders,
settings and NVIDIA DLLs. See START-HERE.txt inside the ZIP.

**Keep your existing loader arrangement.** The inspected Cyberpunk installation
uses Cheeky's `dxgi.dll`, Ultimate ASI Loader's `version.dll`, and RED4ext's
`winmm.dll`. RC5 loads through RED4ext. Overwriting `version.dll` with Cheeky's
alternative loader breaks that ASI-loader arrangement. Disabling `dxgi.dll`
can prevent Cheeky loading. Do not load Cheeky through multiple routes.
A missing optional `dxgi2.dll` alone is not an error.

The ZIP contains no root proxy DLL, NVIDIA DLL, active settings INI, port DLL,
or OpenXR-layer registration. Existing overlay configuration is preserved;
`[Overlay] MenuKey=122` in `CheekyFoveatedDLSS/CheekyOverlay.ini` selects **F11**.
F10 belongs to the VR port. Loader, host and standalone logs are documented.

## Validation and limitations

Release x64 build; native D3D12 and Streamline GPU readback, NR lifetime and
cross-queue ordering, UEVR lifecycle, Vulkan regression, overlay and runtime-host
tests passed. LuaJIT 2.1 and Lua 5.4 menu tests passed. The test archive was
hash-verified and installed with protected-file verification.
Tests use controlled fake NR output; they do not prove improved NVIDIA image
quality, performance or long-session headset stability.

- This is the current tested fork code, not a confirmed fix for the user's
  historical RC5 startup failure. That failure was not reproduced.
- **Avoid extremely low NR working scales.** 12% produced 60x58 per-eye NR
  inputs and two GPU hangs in Cyberpunk. Returning to 78% restored operation.
  The precise cause and a safe minimum are not established; no minimum-size
  guard was added in this build.
- User found high Tone strength suppressed the desired structural effect;
  Tone=0 is a useful comparison baseline, not a universal quality preset.
- Current/previous NR DLL switching still requires restarting the game.
- Added the separate UEVR upgrade on 2026-11-06 after user confirmation that
  the updated menu works. This is not exhaustive gameplay validation of each title.

Archive SHA256:
`81e6baab5dbf268b3626b259362cc0c8ff7db4da981272e2826e09cd91576674`

## UEVR upgrade added 2026-11-06

Download **Cheeky-UEVR-Preview8-20261106.zip** for UEVR games. Close the game,
back up its profile, then merge `plugins` and `scripts` into
`%APPDATA%/UnrealVRMod/<game-executable-profile>/`. Replace the plugin, runtime
and Lua script together; preserve INIs and NVIDIA DLLs. No root DXGI/version
loader or OpenXR registration is needed. Full instructions are inside the ZIP.

The updated Lua menu exposes NR settings while NR is off, adds pass-1 UI
correction, and explains bridge/Lecram dependencies. All 69 standalone NR
setting fields have Lua counterparts; host-specific diagnostics are not claimed
to have complete parity. LuaJIT 2.1 and Lua 5.4 regression tests passed.

Select **NR color pipeline → Linear BT.709 bridge** for Lecram controls and
the percentage **NR output blend** beneath intensity.

UEVR archive SHA256: `5acaadd717c59f1119678fa6bc709a42702aa85f0a4166c4a0bf2026797a819d`
