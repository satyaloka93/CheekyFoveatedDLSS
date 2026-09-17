---
type: architecture
title: Stereo eye roles and the two-box problem
description: How XOffset becomes a mirrored per-eye shift, why GTA showed one box with the same setting, and the LRU thrash that is the leading unconfirmed suspect.
tags: [vr, stereo, foveation, settings]
timestamp: 2026-09-07
---

# Stereo eye roles and the two-box problem

## Cyberpunk separate eye textures (2026-09-15)

Cyberpunk's observed run received valid OpenXR gaze but reported `mapped=0,0`
and `using=0`. Both submitted eyes had local origin (0,0) in distinct resources
and swapchains. The packed-region fallback rejected this layout. The mapping
policy now permits this layout only when CyberpunkVR_Stereo.dll is loaded,
using Cheeky's assigned DLSS eye roles and existing inversion setting. Dimension,
resource validity, stable mapping, focus and gaze validity checks remain active.
Cheeky retains gaze sampling and smoothing. Cyberpunk's native NR geometry
rewrite was already disabled in the observed deployment. In-headset eye order
and projection alignment remain to be verified after deployment.

## Symptom

In VR the foveated region should fuse into **one** box shared by both eyes. It repeatedly appears
as **two** boxes with a seam down the centre.

## How `XOffset` becomes a per-eye shift

`src/settings.cpp` (~line 370) resolves settings per view id:

```cpp
const bool negative = matched_view->second_eye != settings.invert_stereo_x_offset;
result.x_offset = negative ? -settings.x_offset : settings.x_offset;
```

So `XOffset` is **mirrored**: `+X` in one eye, `−X` in the other. `XOffset=0.31` is 62% total
separation between the two eyes' boxes.

## Why the same setting gave one box in GTA

The offset is force-zeroed unless **both** eye roles are latched:

```cpp
if (eye_roles[0].view_id == 0U || eye_roles[1].view_id == 0U) {
    result.x_offset = 0.0F;
    result.nr_x_offset = 0.0F;
    return result;
}
```

Measured distinct view ids seen by the add-on:

| Title | distinct view ids | `XOffset` on disk | observed |
|---|---|---|---|
| GTA San Andreas: TDE | **3** | 0.31 | one box |
| The Outer Worlds 2 | **6** | 0.31 (now 0) | two boxes |

GTA appears never to have latched a second eye role, so its `XOffset=0.31` was inert. The box the
user approved of in GTA was therefore *already* an effective `XOffset=0` — setting TOW2 to 0
reproduces GTA's behaviour deliberately rather than by accident.

## Leading unconfirmed suspect: eye-role LRU thrash

With only two role slots and **six** views presenting, `resolve` evicts by least-recent
evaluation:

```cpp
role_index = eye_roles[0].last_evaluation <= eye_roles[1].last_evaluation ? 0U : 1U;
```

A third view steals a slot from one of the two real eyes, and `second_eye` — hence the **sign of
the offset** — flips for whichever view is reassigned. That is a frame-to-frame sign flip on the
same eye, which is a better explanation for an unstable double image than any static offset.

**Not confirmed.** The add-on does not log eye-role assignment. Confirming it needs a trace of
`view_id → role_index → second_eye` per evaluation. Do not present this as the cause until that
trace exists.

## Fusion geometry — why a hard edge doubles

A hard screen-space edge at the same image coordinate in both eyes carries **zero disparity**, so
it fuses at infinity while the scene behind it sits at a nearer depth. The visual system reports
that conflict as doubling. A nasal-side slab escapes the problem geometrically, because its edges
land in the monocular crescent where there is nothing to fuse against. `TransitionWidth=0`
(TOW2's current value) is the worst case; a non-zero feather softens the cue.

## Current TOW2 state (2026-09-07 16:11 run)

`XOffset=0`, `NrXOffset=0`, `TransitionWidth=0`, `Width=0.5`, `Height=0.4`. The run's log shows
two DLSS-NR features created for two different views, **both** at `region=1600x800@800,624` —
identical source offset, i.e. the zeroing is in effect. ReShade persisted the ini itself at
process exit, so the value is genuine and not a stale hand-edit.

## Cost note, separate from geometry

The add-on creates its **own private DLSS feature** and evaluates it **per eye**, so in VR the
overhead is paid twice per frame. TOW2's internal resolution is low enough (1920×1080 class) that
DLSS is sub-millisecond and a 20% region cannot win the overhead back; Hogwarts at 3840×2160 is
where foveation pays. The thing that produces two boxes and the thing that costs frame time are
the same underlying fact: two views latched, two evaluations.

See [The Outer Worlds 2](../titles/the-outer-worlds-2.md), [GTA San Andreas TDE](../titles/gta-san-andreas-tde.md).
