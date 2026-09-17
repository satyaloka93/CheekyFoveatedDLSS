---
type: architecture
title: NGX dispatcher vs game runtime
description: Why foveation did nothing outside GTA — EvaluateFeature was only ever hooked on nvngx_dlss.dll, never on _nvngx.dll.
tags: [ngx, hooks, minhook, dlss]
timestamp: 2026-09-07
---

# NGX dispatcher vs game runtime

## Two modules, both exporting the same names

| Module | Role | Location |
|---|---|---|
| `nvngx_dlss.dll` | The **game's** DLSS runtime, shipped beside the executable | game `Binaries\Win64` |
| `_nvngx.dll` | The **driver's** NGX dispatcher | `C:\WINDOWS\System32\DriverStore\FileRepository\nvmdi.inf_amd64_*` |

Both export `NVSDK_NGX_D3D12_EvaluateFeature`. This was verified directly against the
`_nvngx.dll` binary — it is not an assumption.

## The defect

`install_direct_export_hooks()` in `src/hooks.cpp` had two blocks:

| block | module | exports hooked |
|---|---|---|
| public runtime | `nvngx_dlss.dll` | 11, **including all four `EvaluateFeature` variants** |
| core runtime | `_nvngx.dll` | **6 only** — `D3D11_Init`, `D3D12_Init`, `D3D12_Shutdown1`, `D3D11_CreateFeature`, `D3D11_ReleaseFeature`, `D3D12_CreateFeature` |

Which module survives to gameplay decides everything:

```text
GTA SA : nvngx_dlss.dll loads and STAYS resident -> public block admitted -> Evaluate hooked -> works
TOW2   : nvngx_dlss.dll LOAD -> UNLOAD twice (~268 ms each), gone before gameplay
         only _nvngx.dll is admitted -> core block has no Evaluate -> 0 crops
```

TOW2's six installed detours matched the core block's six exports exactly. That is the whole bug.

## Second half of the same defect

Even with Evaluate hooked, the private evaluation sourced `create_feature` / `release_feature`
from `real_create_d3d12` / `real_release_d3d12`, which are null once the game runtime unloads.
`evaluate_d3d12_backend` then rejected every frame:

```text
D3D12 canonical rejected ... create=0000000000000000
```

Fix adds a dispatcher fallback:

```cpp
callbacks.create_feature = real_create_d3d12.load(std::memory_order_acquire);
if (callbacks.create_feature == nullptr) {
    callbacks.create_feature = real_core_create_d3d12.load(std::memory_order_acquire);
}
callbacks.evaluate_feature = evaluate;
callbacks.release_feature = real_release_d3d12.load(std::memory_order_acquire);
if (callbacks.release_feature == nullptr) {
    callbacks.release_feature = real_core_release_d3d12.load(std::memory_order_acquire);
}
```

## Shape of the change

The four evaluate detour bodies (51–106 lines each) became `*_common` helpers taking the
original function pointer, so the dispatcher variants reuse them instead of duplicating ~310
lines. Both modules can be admitted at once — GTA admits the runtime **and** the dispatcher — so
a `thread_local bool inside_direct_evaluation`, modelled on the existing
`inside_streamline_evaluation`, stops one frame being cropped twice.

`install_direct_hook` already no-ops when an export does not resolve, so adding the new names to
the core block is safe on drivers that do not export them.

## Status

Commit `280d4d2` on `fixes`. **Verified**: Hogwarts Legacy 34 crops / 0 rejects at 3840×2160;
TOW2 crops with the alignment border visible; GTA unchanged. See [Titles](../titles/).

## A theory that was wrong

The 3-scan module-stability gate was blamed first. It is real — the worker polls every 250 ms
(`WaitForSingleObject(event, 250U)`) and `nvngx_dlss.dll` lives ~268 ms per load, so the gate can
never admit it in TOW2 — but it is **secondary**. Once Evaluate is hooked on the dispatcher, which
is resident for the whole session, the gate stops mattering. Any upstream report must lead with
the missing export, not the gate.
