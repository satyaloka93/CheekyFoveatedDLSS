---
type: title-profile
title: "GTA San Andreas: The Definitive Edition"
description: The title that always worked — reference case and mandatory regression check.
tags: [gta, d3d12, regression, stereo]
timestamp: 2026-09-07
---

# GTA San Andreas: The Definitive Edition

The one title where upstream CheekyFoveatedDLSS worked out of the box. Its value is as a
**control**: anything that changes behaviour here is a regression.

## Why it worked when others did not

`nvngx_dlss.dll` loads and **stays resident**, so the public-runtime hook block is admitted and
`EvaluateFeature` gets hooked. It also admits `_nvngx.dll`, which is why the
`inside_direct_evaluation` re-entry guard exists — with both modules hooked, one frame could
otherwise be cropped twice.

## Stereo

* Add-on sees **3 distinct view ids** (TOW2 sees 6).
* `XOffset=0.31` on disk, same as TOW2 — yet **one box** is observed.
* Explanation: with fewer than two eye roles latched, `settings.cpp` force-zeroes `x_offset` and
  `nr_x_offset`. GTA's `0.31` is inert. See
  [Stereo eye roles](../architecture/stereo-eye-roles.md).

This is the cleanest evidence available for the two-box mechanism: identical setting, opposite
outcome, decided entirely by how many stereo views the title presents to DLSS.

## Regression check (required before any upstream PR)

1. Crops must continue at their previous rate.
2. Crop counts must **not roughly double** — that would mean the double-application guard is not
   catching dispatcher→runtime re-entry.
3. One box, unchanged.
