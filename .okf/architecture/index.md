# Architecture

Cross-cutting mechanics of how the add-on reaches DLSS, and where each one breaks.

* [NGX dispatcher vs game runtime](ngx-dispatcher-vs-runtime.md) — defect 1, fixed and verified.
* [Private feature lifecycle and 0xBAD0000B](private-feature-lifecycle.md) — defect 2, fix built but untested.
* [Stereo eye roles and the two-box problem](stereo-eye-roles.md) — defect 3, cause identified, unconfirmed.
* [AFW compatibility adoption for Cheeky UEVR](afw-uevr-adoption.md) — reference implementation analyzed; staged port recommended.
