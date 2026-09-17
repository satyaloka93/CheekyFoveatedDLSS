---
type: title-profile
title: System Shock Remake
description: The D3D11→D3D12 transport route; crash-on-load solved, 0xBAD0000B fix deployed but never exercised.
tags: [system-shock, d3d11, transport, crt, crash]
timestamp: 2026-09-07
---

# System Shock Remake

`E:\SteamLibrary\steamapps\common\System Shock Remake\SystemShock\Binaries\Win64\`

The only title exercising the **D3D11→D3D12 transport** route, which is where the private
parameter block is allocated by the add-on rather than supplied by the game. ReShade is loaded as
`d3d11.dll` here (renamed by hand).

## Crash on load — solved

Cause: **CRT version mismatch.** The game ships `msvcp140.dll` 14.24; the system CRT is 14.51.
Windows resolves DLL imports from the application directory first, so the add-on bound against
the old runtime. Fixed by copying the system CRT into the game directory with `.game-14.24.bak`
backups.

Five theories were pursued and disproved first: two ReShade copies; UEVR in-process (the crash
log turned out to hold 26 appended runs); a VEH exception storm; a static d3d12 import; callback
registration. The lesson recorded in
[Verification protocol](../operations/verification-protocol.md) is to use real dump tooling
(`tools/crash/read_dump.py`, `analyze_windows_uevr_dump.py`) rather than hand-parsing PE imports.

Dumps: `F:\crashdumps`, captured with procdump. A `-t` termination dump has **no exception
stream** — use `-e 1` to get one. Minidump stream types: 3 ThreadList, 4 ModuleList, 6 Exception.

## `0xBAD0000B` — fix deployed, NOT exercised

Last log on disk (2026-09-05 12:18, 4.9 MB):

```text
Private D3D12 NGX AllocateParameters result=0x00000001
... 27968 x 0xBAD0000B
```

Zero creates succeeded, zero evaluations, no red rectangle, ~200 fps lost to retries.

The capability-parameters fix was built at **12:21** and deployed at **12:21** — three minutes
*after* that log was written. md5 `ce3b143f2b57c9e1dc9b270ec0f2187a` is in place and has never
run. See [Private feature lifecycle](../architecture/private-feature-lifecycle.md).

## Next run must show

| Line | Expected |
|---|---|
| `Private D3D12 NGX GetCapabilityParameters result=` | `0x00000001` |
| canonical create result | `0x00000001` |
| `0xBAD0000B` count | **0** |
| `crop prepare` count | non-zero |
| red alignment rectangle | visible |

If `GetCapabilityParameters` does not appear at all, the export did not resolve and the code fell
back to `AllocateParameters` — a different problem, not a failed fix.

## Config hazard

A whole verification run was wasted with `Enabled=0` and `D3D11D3D12Transport=0` left over from
the crash hunt. **Check both are `1` before running.**
