---
type: title-profile
title: Hogwarts Legacy
description: Strongest verification of the dispatcher fix — 34 crops, 0 rejects, at the resolution where foveation actually pays.
tags: [hogwarts, d3d12, streamline, verification]
timestamp: 2026-09-07
---

# Hogwarts Legacy

## Before the fix

No NGX module observed at all across 12 worker ticks; `setTagForFrame` resolved NULL on an older
Streamline. It looked like a wholly separate failure from TOW2's, and was documented as such.

## After the dispatcher fix

**34 crops / 0 rejects at 3840×2160**, high-resolution motion vectors. This is the strongest
evidence commit `280d4d2` is correct, and the only measurement taken at a resolution where DLSS
is expensive enough for foveation to be worth the private-feature overhead.

## Open

The region is currently 80% × 45% = **36% of pixels**. That is too generous to demonstrate much
saving. Narrowing it is the obvious next measurement, and Hogwarts is the right title for it —
unlike [TOW2](the-outer-worlds-2.md), the arithmetic can work here.
