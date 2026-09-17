---
type: title-profile
title: Silent Hill f
description: User confirms Native Stereo Fix causes eye mismatch; eye tracking remains unresolved.
tags: [silent-hill, d3d12, stereo]
timestamp: 2026-09-08
---

# Silent Hill f

## Current finding — 2026-09-08

User explicitly confirms Native Stereo Fix causes the left/right image mismatch in SHf and is incompatible with their setup. Keep it OFF for this title. Earlier overlapping Cheeky/OFXR experiments do not establish those layers as the cause of this mismatch. Eye tracking remains unresolved.

The successful run's 08:56–08:57 Cheeky log reports `layer=0`; subsequent inspection found user `CHEEKY_OPENXR_LAYER_DISABLE=1` and no OFXR implicit registration. No live game was available for module verification, so the earlier off/off classification was not fully verified.

The old deployment inventory below is historical, not current: a pre-Toolkit-fallback diagnostic addon was restored on September 8 (SHA256 `B0D21E5214140DADB2A7CFA0AE755B334AE7781014E222A698AFB0DBFB555707`). The TOW2 neural-rendering investigation belongs to the sibling UEVR repository: `UEVR-AFW-JOEY/UEVR-AFW-PUBLISH/docs/TOW2_NEURAL_RENDERING_TEST_PLAN.md`.

## Historical inventory — superseded

`F:\SteamLibrary\steamapps\common\SILENT HILL f\SHf\Binaries\Win64\`

Present in the deployment set and carrying `CheekyFoveatedDLSS.addon64` md5
`fa1d405d08b2e1f556470921965175f6`, deployed **2026-09-04 08:55** — before commit `280d4d2`, so
it has **none** of this fork's fixes.

A `CheekyFoveatedDLSS.log` exists but has not been analysed, and no observation from this title
has fed any conclusion in this bundle.

**Deprecation risk:** if this title is not part of the test matrix, redeploy or remove the stale
binary so it cannot be mistaken for a current data point.
