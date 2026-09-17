# Change log

## 2026-09-17

* Integrated upstream AFW with the fork's existing fixes and added a ReShade-owned UEVR projection bridge. Lua and a second renderer are unnecessary on this path. Preserved blink hold while restoring AFW focus-reacquisition history reset. Added focused bridge/core/history regression coverage; game validation remains pending.

* **Consolidation**: The active `fixes` checkout now contains the intended combined Cheeky work and records the Cyberpunk port-side frame-clock correction that restored second-view gaze processing.
* **AFW investigation**: Analyzed `merge/upstream-afw` and documented the required UEVR publication, NGX re-entry protection, verified source-eye metadata, projection-aware coverage/history, and test boundaries. The reference diff is not merged wholesale because it predates current Cyberpunk gaze/stereo fixes.

## 2026-09-15

* Recorded Cyberpunk's valid-gaze/unmapped-eyes failure and the host-scoped
  separate-eye-texture mapping correction in architecture/stereo-eye-roles.md.

## 2026-09-08

* Moved the TOW2 NR plan and preparation script into the sibling UEVR repository's `docs` and `scripts` folders; corrected the SHf reference. Cheeky-specific diagnostics remain here.

* Corrected SHf ground truth from the user's explicit Native Stereo Fix isolation result; marked the old deployment inventory historical. Documented limits of the off/off verification and the isolated UEVR/RenoDX TOW2 plan in repository-root `OPENXR_TEST_PLAN.md`.

## 2026-09-07

* Bundle created. Captured the three defects, per-title ground truth, build/deploy and
  verification runbooks, and the fork's branch policy.
* Recorded that GTA and TOW2 carry the identical `XOffset=0.31` with opposite outcomes, and that
  `settings.cpp` zeroes the offset unless both eye roles latch — the mechanism behind it.
* Recorded the eye-role LRU thrash as the leading *unconfirmed* suspect for the two-box symptom,
  explicitly flagged as needing a `view_id → role_index → second_eye` trace before it is claimed.
* Recorded that the `GetCapabilityParameters` fix is deployed to System Shock but has never run —
  the log on disk predates the binary by three minutes.

## 2026-09-05

* `1b33e30` Set the DLSS creation parameters on the canonical private path. Later found redundant
  with `mirror_transport_parameters()`.
* `280d4d2` Use the NGX dispatcher when the game's DLSS runtime is not resident. Verified in
  Hogwarts Legacy (34 crops / 0 rejects at 3840×2160), TOW2, and GTA unchanged.
* System Shock crash-on-load traced to a CRT version mismatch (game 14.24 vs system 14.51), not
  to any of the five earlier theories.
