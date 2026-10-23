# ADR-0007: EXIF/ICC metadata library (DR-7) — pending

**Status:** Pending — not yet decided
**Plan reference:** PLAN.md §6.4 (IO contracts), task P1.2

## Context

Reading EXIF orientation + metadata, and writing it back on export (FR-3), needs a small library.
Three candidates are on the table:

| Candidate | License | Note |
|---|---|---|
| exiv2 | GPL-2.0+ ⚠️ | Would force GPL on the whole app — rejected unless the app itself goes GPL |
| libexif | LGPL-2.1 | Default plan: read via libexif, write orientation/ICC passthrough ourselves |
| TinyEXIF | BSD | Lighter-weight alternative; narrower feature set |

## Decision

Not yet made. PLAN.md's working default is **libexif for read + our own minimal writer** for
orientation/ICC passthrough, but this ADR stays "pending" until task **P1.2** prototypes both
libexif and TinyEXIF against the fixtures in `tests/golden/fixtures/` (particularly
`exif_rotated.jpg` and `adobergb_tagged.jpg`) and records the outcome here as DR-7.

## Consequences (until resolved)

- `exiv2` must not be added as a dependency anywhere (AGENTS.md §5 rule 7, §14).
- This file should be rewritten (not amended in place — ADRs are immutable once decided, PLAN.md
  §0) once P1.2 lands, with a real decision and rationale, and its `Status` updated to `Accepted`.
