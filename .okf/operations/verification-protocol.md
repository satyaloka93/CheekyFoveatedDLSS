---
type: runbook
title: Verification protocol
description: What counts as evidence a fix works, written after several runs that proved nothing.
tags: [verification, logging, testing]
timestamp: 2026-09-07
---

# Verification protocol

This exists because a run of tests produced identical results repeatedly while being described as
progress. The user's instruction was explicit: iterate on a plan rather than asking for the same
test again.

## The rule

**Every claim of "it works" must name a counter that is non-zero AND a counter that is zero.**

`crop prepare` alone proves nothing. The crop was computed, logged, then discarded for weeks —
the log looked healthy the entire time. Pair it with the reject counter:

| Signal | Good | Meaning if bad |
|---|---|---|
| `D3D12 crop prepare seq=` | increments | Evaluate not hooked → [dispatcher](../architecture/ngx-dispatcher-vs-runtime.md) |
| `D3D12 canonical rejected` | **absent** | callbacks null; crop discarded |
| `create result=` | `0x00000001` | `0xBAD0000B` → [parameter block](../architecture/private-feature-lifecycle.md) |
| red alignment border | visible on screen | nothing is actually being applied |

`AlignmentBorder=1` / `NrAlignmentBorder=1` is the fastest ground truth available. Use it.

## Before every run

1. Game closed during deploy; `md5sum` matches at both ends.
2. `Enabled=1`, and `D3D11D3D12Transport` set for the route under test.
3. Note the current log size/mtime, so lines from the new run can be told apart. **The log is
   appended across runs** — one crash log was read as a single session when it held 26.

## Diagnostics

* Do not gate a diagnostic on a counter used for something else. A diagnostic gated on
  `g_box_applies < 8` never fired because that counter was already in the thousands. Give each
  diagnostic its **own** counter.
* One-shot warnings must be one-shot. A stand-down warning emitted 3,871 lines at 36/sec before
  being converted to a `compare_exchange_strong` log-once.

## Crash analysis

Use the existing tooling — `tools/crash/read_dump.py`, `analyze_windows_uevr_dump.py`. A
hand-rolled PE import parser produced garbage output and sent the investigation down a false
path. Capture dumps with procdump `-e 1`; a `-t` termination dump carries **no exception stream**.

## Anti-patterns, all observed on this project

* Changing a default in code when the config file already carries the key — the change is inert.
* Editing a config file while the process that owns it is running.
* Changing a user's setting mid-investigation "to help" — it invalidates their baseline. Restore
  it.
* Building on a different branch than the one the user is running, and nearly deploying a 1.3 MB
  DLL over a 3.0 MB one.
* Reasoning about a run from a log belonging to a different component.
