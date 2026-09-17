# Titles

Measured per-game behaviour. Every row here comes from a log or a file on disk. Nothing is
inferred; where something is inferred it says so.

* [The Outer Worlds 2](the-outer-worlds-2.md) — dispatcher fix verified; stereo and cost open.
* [GTA San Andreas: TDE](gta-san-andreas-tde.md) — the reference "already worked" title.
* [Hogwarts Legacy](hogwarts-legacy.md) — best evidence the dispatcher fix is real.
* [Cyberpunk VR port second-view frame clock](cyberpunk-vr-port-frame-clock.md) — port-side frame-token correction restored Cheeky gaze processing.
* [System Shock Remake](system-shock-remake.md) — the D3D11 transport route; capability fix untested.
* [Silent Hill f](silent-hill-f.md) — Native Stereo Fix mismatch confirmed by user; eye tracking unresolved.

## Deployed binaries as of 2026-09-07

| Target | md5 | deployed |
|---|---|---|
| repo `bin/Release` (HEAD + uncommitted capability fix) | `ce3b143f2b57c9e1dc9b270ec0f2187a` | built 09-05 12:21 |
| The Outer Worlds 2 | `faf0b1955f30bf7245fec279b0dc342e` | 09-05 09:45 |
| System Shock Remake | `ce3b143f2b57c9e1dc9b270ec0f2187a` | 09-05 12:21 |
| Silent Hill f | `fa1d405d08b2e1f556470921965175f6` | 09-04 08:55 |

TOW2 predates commits `280d4d2` (10:09) and `1b33e30` (11:04) by clock time, but its log shows
`Direct detour installed export=NVSDK_NGX_D3D12_EvaluateFeature` — the build was made and
deployed for testing *before* the commit was authored. TOW2 therefore runs the dispatcher fix
**without** `1b33e30` and **without** the capability-parameters fix.
