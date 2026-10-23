# ADR-0008: Shipped-model training data must have a clean, traceable license (DR-8)

**Status:** Accepted — 2026-10-22
**Plan reference:** PLAN.md §17.3

## Decision

The shipped **WbNet** trains on **Cube++ only** (CC BY 4.0). The shipped **ToneNet** trains on
our own curated **LT-Tone** set only, **from random initialisation**. MIT-Adobe FiveK and PPR10K
are **non-commercial research only** licenses — they may be used solely to select architecture and
hyper-parameters during research, never to produce a shipped checkpoint.

No weights — backbone or head — that ever saw FiveK/PPR10K may be an ancestor of a shipped
checkpoint. `models/manifest.json` records `training_data_license` and `checkpoint_ancestry`
(run IDs back to random init); release CI refuses to package a model whose ancestry includes any
run tagged `research-only`.

## Consequences

- Research experiments and shipped-model training are organisationally separate runs, not just
  separate configs — ancestry tracking must be real, not aspirational.
- A ToneNet architecture validated on FiveK still needs LT-Tone (1,500–3,000 own/CC0/CC-BY
  images, P6.3) collected and labelled before a shippable checkpoint can exist at all.
- Release CI is a hard gate here, not a lint warning (AGENTS.md §11).
