# ADR-0002: Qt 6 Quick/QML for UI, tracking open-source 6.11.x (DR-2)

**Status:** Accepted — 2026-10-22
**Plan reference:** PLAN.md §3 DR-2

## Decision

The UI is Qt 6 Quick/QML. Qt **6.12 LTS is commercial-only** (per Qt's own LTS page), so
open-source builds track the current non-LTS line — **6.11.x** as of this writing. A specific
6.11 patch is pinned and bumped deliberately.

**Avoid GPL-only modules:** Qt Charts (deprecated since 6.10), Qt Graphs, Qt Quick 3D, Qt Quick
Timeline, Qt Virtual Keyboard, Qt Qml Compiler (qmlsc). The histogram and curves are drawn with
`QQuickPaintedItem` or `QSGGeometryNode`, not Qt Charts.

Use the standard `qmlcachegen` (part of qtdeclarative, LGPL) — fine. Do **not** enable
`qmlsc`/"Qt Quick Compiler Extensions" (commercial/GPL).

Rendering goes through Qt RHI (D3D11/D3D12 on Windows, Metal on macOS, Vulkan/OpenGL on Linux).
⚠️ The exact default RHI backend per OS was not independently verifiable from docs; the app does
not depend on it and exposes a `QSG_RHI_BACKEND` override in a diagnostics menu.

## Consequences

- No Qt Charts/Graphs/Quick3D/Timeline/VirtualKeyboard/qmlsc anywhere in the codebase — enforced
  by review, not currently by a CI grep (see AGENTS.md §5 rule 7).
- Qt is linked dynamically only (LGPLv3 obligation) — see ADR for licensing in PLAN.md §13.
- Qt version bumps are a deliberate, documented action (AGENTS.md §12), not an automatic upgrade.
