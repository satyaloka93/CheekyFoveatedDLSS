# Lecram controls (experimental)

Standalone: NR settings > Lecram controls. UEVR: DLSS-NR > Lecram controls.
All settings are shared across passes. Default Off/0 preserves the previous result.
These are independently implemented adaptations informed by static analysis of
renodx-dlss5_lecram.addon64, not exact replicas of its entire v11 processing stack.

Requires a bridge NR color pipeline and the D3D12 NR path. Legacy ignores these
controls; Vulkan is not implemented. Cropped-only transport inputs cannot use
the whole-view meter and keep existing exposure; the log records requested and
active state. No NR model-weight edits or skipped-evaluation correction reuse are included.

| Setting / INI key | Meaning | Starting test |
| --- | --- | --- |
| Automatic input exposure / NrLecramMeter | A 64x36 whole-view GPU sample measures log-average luminance and maps it toward 0.18, with gain bounded 1/16..16. Replaces game exposure; manual multiplier/reference white remain active. All-black input uses gain1. No history or CPU readback. | Enable alone; compare the same scene. |
| Temporal detail stability / NrLecramStability | Motion-reprojects and blends previous NR correction, with depth/brightness rejection and current-neighborhood clamping. 0 = off, 1 = strongest (up to80% prior weight on accepted pixels). NR evaluates every frame. | Start0.3; watch moving faces, hair and disoccluded edges. |
| Near-black lift protection / NrLecramBlackGuard | Reduces positive NR luminance changes approaching black (0..0.02 reference-white units). Scales the RGB correction together. | 0.5, then compare0. |
| Bright-edge darkening protection / NrLecramEdgeGuard | Reduces negative NR luminance changes on bright pixels next to darker neighbors, using a3x3 original-image neighborhood. May weaken intentional shadows. | 0.5, then compare0. |

Reset Lecram controls sets Off/0/0/0. In the runtime settings INI:

```ini
NrLecramMeter=false
NrLecramBlackGuard=0
NrLecramEdgeGuard=0
NrLecramStability=0
```

The meter uses one small compute dispatch per enabled pass/evaluation and a1x1
RGBA32F scratch texture per cached NR resource set (typically64KiB allocation).
The edge guard adds neighborhood reads only when nonzero. Neither option saves
NR evaluations or promises faster performance. Normalization can alter what the
model generates; restoration reverses normalization rather than simply applying
exposure to the screen. No temporal exposure governor is included yet.

Logs: DLSS-NR Lecram reports view, color pipeline, requested/active metering,
and both guard strengths when NR history resets. The existing exposure status
identifies the whole-view GPU meter when active. These indicate execution setup,
not measured GPU time or a claim of visual improvement.

## Temporal stabilization test

Cyberpunk first. Keep the current appearance/network/pass/scale settings, then
compare Temporal detail stability0 against0.3 in the same scene while moving
normally. Check talking faces, hair/foliage, edges revealed by head turns and
looking between near/far objects. If lag or soft trails appear, return to0.

Histories are separate for each eye and pass. Fresh NR is used when there is no
compatible submitted preceding history, after reset, or when per-pixel motion,
depth or brightness tests reject the old sample. Moving crops use the existing
NR motion-guide compensation. Raw pipeline diagnostics bypass this filter so
original/proxy/neural/restored captures retain their established contract;
ordinary final-image screenshots include it.

The new filter costs one extra working-resolution compute dispatch per pass,
plus history bandwidth. Histories use FP16 RGB correction + input luminance and
R32 depth, at most3 slots per eye/pass, bounded globally to384MiB of actual
texture allocations. Descriptors/fences are additional small allocations.
No temporal resources are allocated before enabling it. Off releases histories
as their recordings retire and GPU work completes, not immediately mid-flight.
Budget or setup failure leaves fresh NR active. This is a quality experiment,
not a performance optimization. It can coexist with F5, which is a separate
internal model feature.

`DLSS-NR temporal` log lines report strength, historyEligible/fresh/fallback
frame counts, history bytes and reason. Eligible means history was supplied to
the shader; it does not count how many pixels passed rejection. Existing NR
GPU timing includes filter work. No on-device quality benefit is established
by fixture tests.
