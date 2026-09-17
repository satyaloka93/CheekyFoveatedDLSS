---
type: overview
title: What this fork is fixing
description: Entry point — the three defects under investigation, and which are proven fixed.
tags: [dlss, ngx, reshade, foveation, vr]
timestamp: 2026-09-07
---

# What this fork is fixing

Upstream CheekyFoveatedDLSS foveates DLSS from a ReShade add-on: it shrinks the region DLSS
actually upscales/denoises to a box around the centre of vision and leaves the periphery alone.
It worked in GTA San Andreas: TDE and did nothing at all in several other titles.

Three separate defects were found. They are independent — do not treat progress on one as
progress on another.

| # | Defect | State | Concept |
|---|---|---|---|
| 1 | `EvaluateFeature` never hooked on the NGX **dispatcher**, only on the game runtime | **Fixed and verified** (Hogwarts, TOW2, GTA) | [NGX dispatcher vs game runtime](architecture/ngx-dispatcher-vs-runtime.md) |
| 2 | Private D3D12 feature created from an empty parameter block → `0xBAD0000B` forever | Fix built, **never tested** | [Private feature lifecycle](architecture/private-feature-lifecycle.md) |
| 3 | Two boxes instead of one fused stereo box in VR | Cause identified, **not confirmed** | [Stereo eye roles](architecture/stereo-eye-roles.md) |

## The one thing that made defect 1 invisible for so long

The crop was computed correctly, logged correctly, and then **thrown away**. The private
evaluation path rejected the frame and restored the original parameters, so the game upscaled
the full frame with no error, no border, and no frame-time change. A log full of plausible
`crop prepare` lines therefore proved nothing. See
[Verification protocol](operations/verification-protocol.md) — every claim of "it works" must
name a counter that is non-zero *and* a counter that is zero.

## Ground rules learned the hard way

* Deploy with the game **closed**, then `md5sum` both ends. A silent `Permission denied` looks
  exactly like a fix that did not work.
* ReShade **rewrites `ReShade.ini` on process exit**. Editing it while the game runs loses the
  edit. Three test runs were burned on the equivalent mistake in UEVR's `config.txt`.
* A log file is appended across runs. Check timestamps before attributing a line to the run you
  just did.

See also: [Fork and branch policy](decisions/fork-and-branch-policy.md).
