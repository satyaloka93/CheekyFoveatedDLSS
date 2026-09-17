---
type: integration record
title: Cyberpunk VR port second-view frame clock
description: The port-side frame-token correction restored Cheeky gaze processing for Cyberpunk's second view.
tags: [cyberpunk, vr, streamline, eye-tracking]
timestamp: 2026-09-17
---

The intended Cheeky baseline is the combined local history containing the
published `pr/performance-blink`, `fix/uevr-no-nr-host`, and `fix/nr-codec-reuse`
work. The active `fixes` branch now contains those commits plus local tested
diagnostic changes; the old toolkit-fallback experiments are preserved on
`archive/main-before-consolidation-20260916`.

The visible Cyberpunk failure was in the separate port, not in Cheeky's branch:
Cyberpunk's offscreen view kept supplying frame zero while the main view
advanced. The port now pairs the second view's constants upload and evaluation
with the advancing main frame and keeps its upload-deduplication marker separate.
The user confirmed eye tracking returned afterward.

This record does not claim automatic marker calibration succeeded. Shared gaze
remains the working Cyberpunk mode. The `-upstream-pr` checkout must remain
available until its worktree registration is removed; a consolidation backup
was created before switching the first folder to this baseline.
