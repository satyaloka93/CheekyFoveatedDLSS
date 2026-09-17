---
type: architecture
title: Private feature lifecycle and 0xBAD0000B
description: The add-on's own DLSS feature fails to create on the D3D11 transport route because its parameter block was allocated empty rather than obtained from the driver.
tags: [ngx, dlss, d3d11, d3d12, parameters]
timestamp: 2026-09-07
---

# Private feature lifecycle and `0xBAD0000B`

## Measured symptom

System Shock Remake, 2026-09-05 run: **27,968 consecutive `0xBAD0000B`** in
`CheekyFoveatedDLSS.log`. Zero successful creates, zero evaluations, zero crops. No red
alignment rectangle ever appeared, and the retry cost roughly 200 fps.

`0xBAD0000B` is `NVSDK_NGX_Result_FAIL_MissingInput`.

## Relevant NGX result codes

| Code | Meaning |
|---|---|
| `0xBAD00002` | FeatureNotSupported |
| `0xBAD00005` | FeatureNotFound |
| `0xBAD0000B` | **FAIL_MissingInput** |

## Cause

Two ways to obtain an NGX parameter block:

| Export | What you get |
|---|---|
| `NVSDK_NGX_D3D12_AllocateParameters` | an **empty** map |
| `NVSDK_NGX_D3D12_GetCapabilityParameters` | a block the driver has **already filled** with feature-availability and scratch-buffer fields that `CreateFeature` reads |

A native D3D12 title never trips over this, because the block handed to `CreateFeature` is the
**game's** — and the game obtained it the capability way. The D3D11→D3D12 transport allocates its
own private block, got it from `AllocateParameters`, and so `CreateFeature` answers
`FAIL_MissingInput` on every attempt, forever.

## Fix

`src/hooks.cpp` resolves `NVSDK_NGX_D3D12_GetCapabilityParameters` into
`ngx.capability_parameters`; `src/d3d11_d3d12_transport.cpp` prefers it and falls back to
`AllocateParameters` only when the export is absent, so behaviour is unchanged on a runtime that
does not offer it. The trace names which one was used:

```text
Private D3D12 NGX GetCapabilityParameters result=0x00000001 exception=0x00000000 parameters=...
```

## Status — UNTESTED

Built as `bin/Release/CheekyFoveatedDLSS.addon64` md5 `ce3b143f2b57c9e1dc9b270ec0f2187a`,
deployed to System Shock Remake 2026-09-05 12:21. **The System Shock log on disk is from 12:18 —
it predates the build.** Nothing has exercised this code. Do not describe it as fixed.

Expected on a good run: `GetCapabilityParameters result=0x00000001`, a canonical create result of
`0x00000001`, and non-zero crops. See
[Verification protocol](../operations/verification-protocol.md).

## A change that turned out to be redundant

Commit `1b33e30` adds four creation parameters (node masks, quality value, create flags) to the
canonical private create path. `mirror_transport_parameters()` in
`src/d3d11_d3d12_transport.cpp` (~line 1177) **already sets all four** on its own destination
block, along with jitter, MV scale and presets. The commit is inert on the transport route and
should be dropped or re-justified before any upstream PR — it was written under the belief that
missing creation params caused `0xBAD0000B`, which the capability-block finding supersedes.

## Related crash, now closed

System Shock crashed on load for five wrong reasons before the real one: **CRT version
mismatch**. The game ships `msvcp140.dll` 14.24; the system CRT is 14.51, and Windows resolves
DLL imports from the application directory first, so the add-on bound against the old one.
Resolved by copying the system CRT in with `.game-14.24.bak` backups. Disproved theories, for the
record: two ReShade copies; UEVR in-process (the crash log held 26 appended runs); a VEH
exception storm; a static d3d12 import; callback registration.
