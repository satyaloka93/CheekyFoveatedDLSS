# Cyberpunk integration — 2026-09-15

## Stereo cache isolation (2026-09-16)

The projection candidate did not resolve the user's one-eye-only circle. The
11:05–11:09 run showed moving converted gaze on both views, repeated viewport 1
cropped-constants rejection, and zero valid pairs from 1,465 calibration captures.
Peripheral DLAA did run at 891x891 in early evaluations; that is not proof of
correct final-eye composition throughout gameplay.

The next candidate selects SR tags/constants by evaluated viewport and frame,
caches options per viewport, uses matching options for center/peripheral passes,
and stamps the evaluated viewport's output for calibration. Failed constants
writes invalidate our accepted-constants cache: upstream Streamline can replace
stored data while returning a duplicate error. No error is suppressed.

`CheekyTests.exe --streamline-stereo` checks alternating eye IDs through a reused
input pointer and rejection of unknown/ambiguous input handles. `SL STEREO`
logs report per-viewport native-path, successful foveated/peripheral, and failed
attempt counts. Successful command recording is not proof of visible pixels.
Normal per-eye gaze mapping is not yet validated; CenterMode=3 is retained
unchanged rather than claiming calibration now works. Headset result pending.

## Gaze projection and repeated constants (2026-09-16)

The shared-gaze experiment was observed in only one eye and too far left.
The recorded right-eye UV near 0.327 was being used directly in the symmetric
game projection. Shared mode now converts through view-space tangents; the
recorded sample converts near render center (0.486). A missing Cyberpunk eye
projection may reuse only a symmetric camera seen within 100 ms in the same
XR session. This assumes parallel render-eye axes and needs headset validation.

Streamline error 0x1B is eErrorDuplicatedConstants (NVIDIA sl_result.h).
The addon now avoids resubmitting byte-identical constants already accepted
for the same frame and private viewport. Changed or unknown constants still
require successful submission; this does not suppress arbitrary 0x1B errors.
The focused `CheekyTests.exe --cyberpunk-gaze` checks projection conversion,
direction, missing projection and constants reuse boundaries. It passes;
`--present-lock` also passes. Both-eye visual correctness remains unverified.

## Captured startup deadlock

Dump `cyberpunk-vr-port/build/cheeky-title-freeze-35920.dmp` captures
Present blocked in `collect_dlss_nr_input_submissions` waiting for the execution
mutex, while the XR thread enters ReShade's ExecuteCommandLists and waits on
its queue lock. ReShade owns that queue lock while invoking Present callbacks.
`note_d3d12_present_impl` and `eye_calibration_tick` now try the execution mutex
and defer maintenance if busy. This retains retirement/fence checks while
avoiding a Present/execute lock inversion. In-game confirmation is pending.

The active source tree is `CheekyFoveatedDLSS-upstream-pr`, remote `fork`
(`satyaloka93/CheekyFoveatedDLSS`), based on local commit `fac862f`.
The September 14 addon deployed in Silent Hill f and The Outer Worlds 2
has SHA256 `572b6420bbd00d509244daa08f09e1c26a3a20538d55e67884e30c7dce458449`.
The similarly named `CheekyFoveatedDLSS` directory is an older tree.

Cyberpunk received valid gaze but rejected both DLSS-to-XR eye mappings.
It renders two DLSS views through a shared intermediate and submits separate
eye-local OpenXR textures. Do not use the old tree's eye-order fallback:
the user observed tracking in only one eye, switching eyes with inversion.
Correct eye calibration remains to be confirmed in game.

The current changes make NR/gaze/calibration diagnostic reads nonblocking
and acquire the GPU execution lock before calibration state in GPU-touching
calibration paths. The old mixed order permitted a state/execution lock cycle.
This is a source-level correction, not proof of the reported freeze's cause:
the process had exited before a thread dump could be captured.

Every five seconds, Cyberpunk's addon log now reports
`CYBERPUNK EYE CALIBRATION` with capture, rejection and mapping counters.
This avoids needing the ReShade menu to diagnose calibration.

Validation: the updated core test run passed, including hardware and WARP
eye-calibration tests and NR lifetime/processing checks; the overall build script
failed afterwards on the separate UEVR test `Parse Cinematic style`.
Do not describe that overall script as passing. Keep existing `uevr/runtime.cpp`
work intact. Only deploy the addon to Cyberpunk; other games retain their builds.
