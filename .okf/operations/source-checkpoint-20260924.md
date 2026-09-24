---
type: checkpoint
title: September 24 Cheeky source publication
description: Publish the maintained fixes tree with AFW border handoff and Native cost diagnostics, preserving older fork history without restoring superseded implementations.
timestamp: 2026-09-24
tags: [git, deployment, afw, diagnostics]
---

# Scope

The maintained checkout is `F:\ai\GITHUB\CheekyFoveatedDLSS`, branch
`fixes`, published to remote `fork` (`satyaloka93/CheekyFoveatedDLSS`).
Local baseline `9426aa7` includes the consolidated gaze/stereo fixes and
ReShade-owned UEVR AFW integration. This checkpoint adds:

- One visible AFW composite mask while retaining donor allocation coverage;
  fixed-position SR/NR mask finalization and stale gaze-mask prevention.
- Exact-hash recognition of the installed official beta.4 runtime.
- Optional `CheekyAFWBorderV1` handoff so a compatible UEVR OpenXR host draws
  the SR diagnostic outline after the eye copy rather than into warp history.
- Five-second and toggle-triggered GPU cost logging with sample counts,
  publication age and timer pipeline counters.
- Focused regression tests and the associated operational documentation.

# History reconciliation

The fork's old `fixes` tip is `1b33e30`, with two commits absent from the
consolidated local ancestry. Preserve them through a tree-preserving merge:

- `280d4d2`: superseded by current `hook_core_evaluate_d3d12`,
  `dispatch_d3d12_ngx_evaluation` and route-specific backend callbacks.
- `1b33e30`: redundant transport creation-parameter writes, as recorded in
  [private feature lifecycle](../architecture/private-feature-lifecycle.md).
  Current transport still sets node masks, quality and creation flags.

The merge deliberately retains the consolidated source, not the older
implementations. No force-push, upstream PR, release asset or game deployment
is part of this source publication. The volatile local `WORKLOG.md` is excluded.

# Deployed binaries and limits

At the September 24 inspection, TOW2's addon matches the September 19 local
Release build: SHA-256
`e0e69edea5fd650112befe006f5041dd08fa62cbdb45b6910192008ef63a373f`.
SHf retains the preceding final-eye-border build:
`42a81b9aa56d37d19c79e47111eac00b408c62b3ad31e37a51ebf03b91271500`.
SHf does not have the later cost logging. Neither installed file is changed
by committing or pushing.

September 24 verification: Release build passed (one C4127 warning in
`hooks.cpp`); addon SHA-256 remains identical to the deployed TOW2 build.
Focused `--cost-diagnostics`, `--afw-core`, `--d3d12-history`,
`--d3d12-composite` and `--present-lock` tests passed again.
The no-argument test suite
previously stalled; this is not a full-suite pass. Final-eye border visual
validation and AFW ghosting remain unresolved, and no FPS gain is claimed.
See [AFW behavior and tests](afw-single-region.md) and
[cost logging semantics](native-cost-logging.md).
