---
type: decision
title: Fork and branch policy
description: One branch, nothing unverified reaches a remote, no automatic pushes.
tags: [git, policy, upstream]
timestamp: 2026-09-24
---

# Fork and branch policy

Standing instructions from the repository owner. These override default agent behaviour.

## Remotes

| Name | URL |
|---|---|
| `origin` | `https://github.com/ClarkCheekyKent/CheekyFoveatedDLSS.git` (upstream) |
| `fork` | `https://github.com/satyaloka93/CheekyFoveatedDLSS.git` |

## Rules

1. **One branch — `fixes`.** Not one branch per fix. Splitting into `fix/a` and `fix/b` for tidy
   upstream PRs made the owner carry two rebases and two builds for a benefit that was the
   maintainer's. Separable commits on a single branch already give him that option.
2. **Never push without being asked.** Commit locally, report what was committed, stop. Earlier
   permission for one push does not carry to the next.
3. **Nothing unverified reaches a remote**, even when a push *was* requested. A request to set up
   a fork is not permission to publish work that has never passed a test.
4. **No `Co-Authored-By: Claude` trailer** in commit messages. Ever.
5. **No upstream PR** until the owner says so, and not before the
   [GTA regression check](../titles/gta-san-andreas-tde.md) passes.

## Current publication checkpoint

See the [September 24 checkpoint](../operations/source-checkpoint-20260924.md)
for the consolidated source, older fork-history reconciliation, deployed
binary differences and test limitations. The state below is historical.

## Historical state — 2026-09-07

Branch `fixes`, ahead of `origin/master` by 2:

| Commit | Subject | Verified? |
|---|---|---|
| `280d4d2` | Use the NGX dispatcher when the game's DLSS runtime is not resident | **yes** — Hogwarts, TOW2, GTA |
| `1b33e30` | Set the DLSS creation parameters on the canonical private path | **no** — and believed redundant |

Uncommitted in the working tree: the `GetCapabilityParameters` fix across `src/hooks.cpp`,
`src/d3d11_d3d12_transport.{cpp,hpp}`. Built and deployed to System Shock, never run.

## Upstream report

A draft issue report existed in a scratchpad and has been lost to cleanup. It led with the
module-stability gate, which is **secondary**; any rewrite must lead with the missing
`EvaluateFeature` export on `_nvngx.dll`. See
[NGX dispatcher vs game runtime](../architecture/ngx-dispatcher-vs-runtime.md).

Keep working files in the repo or in `.okf`, not in `/tmp` — a scratchpad was wiped mid-project
and took the draft with it.
