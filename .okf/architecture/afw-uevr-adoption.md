---
type: Architecture Note
title: AFW compatibility adoption for Cheeky UEVR
description: AFW support requires coordinated UEVR publication, NGX routing, and source-eye metadata rather than a settings-only integration.
tags: [afw, uevr, ngx, d3d12, foveation]
timestamp: 2026-09-17T00:00:00Z
---

# Current source boundary

The AFW implementation has now been integrated into the `fixes` working tree
from upstream `a1c2a11`, preserving the fork's Cyberpunk and resource-lifetime
changes. ReShade owns processing; the UEVR plugin forwards copied frame data
through the versioned `CheekyAddon_PublishUEVRFrame` export, acquiring and
releasing a module reference per call. No Lua or second runtime is needed on
this route. See `uevr/README.md` and `scripts/test-afw-reshade.ps1`.

The earlier investigation used the
remaining `merge/upstream-afw` worktree contains the reference implementation
at `42e0ae0`, built around the AFW feature sequence `8000e2e` through
`d2e146e` and later non-AFW compatibility fixes.

The AFW path is DX12-only and is designed to work with an unmodified public
AFW UEVR build. It is separate from the OpenXR gaze layer for fixed or
automatic coverage; runtime gaze still needs the matching gaze source.

# What must be adopted together

1. **UEVR publication boundary.** `uevr/plugin.cpp` publishes UEVR's two eye
   projection matrices, HMD dimensions, and `VR_RenderingMethod` through
   additive `CheekyUEVR_PublishStereo` and `CheekyUEVR_PublishRenderingMode`
   exports. The plugin rejects unstable matrix/size reads and clears the
   projection on non-DX12 or inactive-headset frames.
2. **AFW-aware NGX dispatcher.** `d3d12_ngx_dispatch.*` distinguishes the
   AFW outer/core evaluation from Cheeky's nested private SR/NR evaluation,
   prevents private work from re-entering the AFW full-frame hook, and allows
   standalone public NGX processing only when AFW is not selected.
3. **Warp observation and source-eye identity.** `hooks.cpp` detects
   `PDAFWPlugin.dll`, verifies the known beta-6 warp ABI by module hash before
   reading metadata, and observes `EvaluateFrameWarp`. Explicit AFW source-eye
   metadata is bound to the submitted depth resource; frame parity or handle
   guesses are deliberately not used.
4. **Projection-aware coverage/history.** `afw_compatibility.hpp` maps both
   eye regions into the verified source projection, applies fixed/automatic/
   bilateral-gaze coverage, and keeps allocation dimensions stable during gaze
   motion. AFW private SR/NR histories are reset after skipped/failed passes or
   a real mode/size transition.
5. **Diagnostics and tests.** The UEVR snapshot/support report exposes AFW
   routing, warp age, ABI status, source-eye counts, projection validity, and
   effective coverage. The CMake suite includes active/inactive AFW, missing
   lower-route, runtime-discovery, history, and menu tests.

# Original adoption plan

The implementation needs more than the Lua controls or UEVR exports. Port the
dispatcher/re-entry guard and AFW runtime observer into the current `fixes`
hook architecture, then add projection publication, source-eye depth binding,
and AFW coverage/history handling. Re-run the AFW fixture tests before any
headset deployment. The large diff from `merge/upstream-afw` should be treated
as a reference patch, not merged wholesale, because it predates the current
Cyberpunk gaze/stereo isolation fixes.

# Runtime safety rules

- Unknown AFW binaries may be observed and routed conservatively, but their
  parameter layout must remain opaque.
- AFW DLL presence alone must not disable ordinary Native Stereo/AFR or DX11
  processing.
- The original game full-frame evaluation remains owned by AFW; Cheeky owns
  only the nested private SR/NR pass.
- AFW coverage is valid only while UEVR has published a recent DX12 projection
  and rendering mode; stale data falls back to centered coverage.

# Validation scope

The focused suite exercises the actual UEVR plugin with a mock addon receiver,
including initialization without Lua, projection/mode forwarding, reset and
module release. AFW core tests exercise routing, gaze, projection expiry and
coverage; GPU history tests cover alternate-eye crop correction. Cyberpunk
stereo/gaze and nonblocking Present checks are retained.

The general standalone UEVR suite still expects NR keys removed by this fork
and fails at `Parse Cinematic style`. It is not evidence for the ReShade bridge
and has not been declared passing. Actual NVIDIA inference and headset output
are outside the focused fixtures. No game deployment was performed.
