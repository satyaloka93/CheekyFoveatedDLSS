---
type: diagnostic
title: Native VR GPU cost logging
description: Periodic GPU means with sample freshness and SR/NR state transitions for correlating Cheeky with UEVR copy costs.
timestamp: 2026-09-19
tags: [performance, diagnostics, uevr]
---

`src/addon.cpp` Present now writes `[Cheeky cost]` every five seconds and when
observing SR/NR enable-state changes, including menu toggles. No rendering or
timing-sampling policy changed. Logs include existing D3D12 native DLSS,
foveated DLSS and peripheral GPU means plus cumulative published sample counts
and time since publication. No published sample has age -1. Require fresh age
and advancing counts before treating a value as current; averages persist
across enable/disable and the first transition record may describe prior work.
Present-thread-local cadence avoids adding a hot-path logging mutex; multiple
Present threads can emit their own initial/periodic records.

GPU values are per measured evaluation (often one eye), not automatically
full stereo frame cost. Existing scopes can overlap; do not blindly sum them.
Submission/completion/publication/failure and pending counters expose a
nonfunctional GPU timer rather than allowing stale/zero means to look free.

UEVR-AFW-PUBLISH's companion copy timer adds sampled final-copy GPU spans and
CPU waits to its log. These do not cover every UEVR operation or Unreal pass.
Deployment is scoped to the TOW2 addon and shared UEVR backend, with backups;
SHf addon and all user settings remain unchanged. Runtime validation pending.

TOW2 deployment `20260919-193839` verified addon SHA256
`e0e69edea5fd650112befe006f5041dd08fa62cbdb45b6910192008ef63a373f`;
old addon retained as `.pre-native-costs-20260919-193839.bak`. Release build
and focused `--cost-diagnostics`, `--afw-core`, `--d3d12-history`,
`--d3d12-composite`, `--present-lock` passed. Full no-argument suite stalled
without output and was stopped (previously documented limitation); do not
claim a full-suite pass. Freshness regression checks unavailable values,
published mean/count, aging without new samples, and invalid metric handling.
