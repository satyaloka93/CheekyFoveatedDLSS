# Cheeky NR bridge preview 7 — Cyberpunk SBS flashing fix

Fixes intermittent foveation flashes in Cyberpunk VR Port SBS rendering when
Cheeky's eye-camera mapping temporarily reverted to the symmetric main camera.
The correction uses each eye's published window and the current evaluation's
vertical field of view, including when the main-camera cache entry is evicted.
Unrelated auxiliary-camera updates no longer invalidate stable eye windows.

The user reported that this correction resolved the observed flashes in gameplay.
This does not claim to fix every possible flashing artifact.

## Installation

These are upgrades for existing working installations, not complete installers.
Close the game and back up the files you replace.

- **Standalone-Upgrade:** merge `CheekyFoveatedDLSS/` into the directory beside
  the game executable. For Cyberpunk this is `Cyberpunk 2077/bin/x64/`.
  Replace Host and Runtime together. Preserve your existing loader, INIs,
  NVIDIA DLLs and Cyberpunk VR Port files.
- **UEVR-Upgrade:** merge `plugins/` and `scripts/` into the existing profile at
  `%APPDATA%/UnrealVRMod/<game-profile>/`. Replace the plugin in its existing
  location; do not leave a second copy loaded. The camera correction targets
  Cyberpunk SBS; this release does not claim a new UEVR-specific flashing fix.

Keep your existing overlay menu key. If its INI is missing, run the included
`Setup-CheekyMenu.ps1` beside the game executable. It creates an F11 default and
preserves an existing file. To choose F11 manually, set `MenuKey=122` under
`[Overlay]` in `CheekyFoveatedDLSS/CheekyOverlay.ini`, then restart the game.
See `OVERLAY-MENU.txt` for controller controls and diagnostic logs.

No NVIDIA DLLs, personal tuning INIs, OpenXR layer installer or port DLL are
included. No registry changes are needed for this upgrade. Each ZIP includes
SHA256SUMS.txt. To roll back, close the game and restore your backup.

## Scope

Built on preview 6, retaining its NR memory retirement and old/new NR DLL switch
with separate tuning profiles. Switching NR DLLs requires restarting the game.
See RELEASE-PREVIEW6.md for memory-counter limitations and cleanup instructions.

This release does not include the experimental NR grain filter, Cyberpunk RR
bypass, or the experimental port stereo-RR changes. It does not resolve RR
quality/performance or establish the cause of high total VRAM usage. It is the
maintained Chtiblue-based fork, not upstream 0.6.3.

## Validation

Release builds and focused regression results are recorded in the GitHub release
notes. Camera tests cover MAIN cache eviction, asymmetric eye windows, repeated
correction, zoom and invalid inputs. The gameplay confirmation applies to the
same correction; the packaged ZIPs have not received a separate headset test.

Credits: ClarkCheekyKent, Chtiblue, janblade/OptiScaler contributors and this fork.
GPLv3 source is available at this release tag.
