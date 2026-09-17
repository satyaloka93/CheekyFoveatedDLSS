---
type: runbook
title: Build and deploy
description: How to produce the add-on and get it into a game without silently deploying nothing.
tags: [build, deploy, msbuild, reshade]
timestamp: 2026-09-07
---

# Build and deploy

## Build

```powershell
scripts\build.ps1 -Configuration Release
```

MSBuild via vswhere. Output: `bin\Release\CheekyFoveatedDLSS.addon64` (+ `.pdb`).

## Deploy

Copy the `.addon64` next to the game's ReShade DLL — the same directory as `ReShade.ini`, which
for UE titles is `<Game>\Binaries\Win64\`, **not** the game root.

Three rules, each learned from a wasted run:

1. **Close the game first.** A running process holds the file and the copy fails with
   `Permission denied`. A silent failed copy is indistinguishable from a fix that did not work.
2. **`md5sum` both ends afterwards.** This is the only proof the deploy happened.
3. **Keep the upstream binary.** `CheekyFoveatedDLSS.addon64.upstream-2.2.bak` is preserved in
   TOW2 for A/B comparison.

## Settings hazards

* ReShade **rewrites `ReShade.ini` on process exit**. Any hand-edit made while the game is
  running is lost. Edit with the game closed; after a run, the on-disk ini is authoritative
  because ReShade itself wrote it.
* Check `Enabled=1` before every test run. It has been left at `0` from earlier debugging and
  cost a full verification run.
* `[ADDON] DisabledAddons` in `ReShade.ini` silently suppresses an add-on. Check it before
  concluding the add-on failed to load.

## Interaction with other add-ons

RenoDX (`renodx-dlss5.addon64`) coexists in TOW2. It was a useful control — it worked in the same
process, same ReShade, same session, while Cheeky did nothing, which localised the fault to
Cheeky's hooking rather than the environment. Disabling it is a valid isolation step.

## Environment

The add-on is exercised inside **UEVR** for VR titles. UEVR's log and `config.txt` live with the
UEVR backend, **not** in the game folder. UEVR rewrites `config.txt` on exit, exactly like
ReShade.

See [Verification protocol](verification-protocol.md).
