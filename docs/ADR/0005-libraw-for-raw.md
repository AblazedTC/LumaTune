# ADR-0005: LibRaw 0.22.2 for RAW decoding (DR-5)

**Status:** Accepted — 2026-10-22
**Plan reference:** PLAN.md §3 DR-5, §15 (V2 design)

## Decision

LibRaw is dual-licensed LGPL-2.1 / CDDL-1.0; it is used under **LGPL-2.1 with dynamic linking**.

Confirmed `libraw_output_params_t` fields needed: `output_bps=16`, `gamm[0]=gamm[1]=1.0`,
`no_auto_bright=1`, `use_camera_wb=0`, `user_mul[4]`, `exp_correc`/`exp_shift`/`exp_preser`,
`highlight`, `output_color`, `half_size`.

LibRaw's `dcraw_process` output is **bounded** 16-bit, so headroom, highlight reconstruction,
white balance, and exposure must be applied **inside LibRaw** before quantisation — there is no
sensor headroom left to recover afterward. See PLAN.md §15.2 for the two-pass decode this implies.

## Consequences

- RAW white balance and exposure are not a pass-through app-side operation like JPEG/PNG; the
  engine's WB/Exposure stage slots for RAW documents are pass-through, filled from LibRaw's
  decode-time diagnostics instead of computing anything themselves.
- Dynamic linking only; `THIRD_PARTY_NOTICES.md` must carry the LGPL-2.1 notice and source offer.
- One `LibRaw` instance per job (thread-safety by isolation), not a shared instance.
