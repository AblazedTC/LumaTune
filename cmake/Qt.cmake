# Pinned Qt version (AGENTS.md §3): a specific identified 6.11.x patch,
# not a floating major-only version and never 6.12 LTS (commercial-only).
# Minimum, not EXACT, version match - robust against aqtinstall resolving
# a slightly newer 6.11.x patch than this file names; CI's install step
# and this pin should still both track the same 6.11.x line by hand.
set(LUMA_QT_VERSION "6.11.2")

# QT_ROOT (the kit dir, e.g. .../6.11.2/msvc2022_64) is how AGENTS.md's
# documented manual-install workflow points CMake at Qt; CI instead lets
# jurplel/install-qt-action set Qt6_DIR directly, so this is a no-op there.
if(DEFINED ENV{QT_ROOT} AND NOT DEFINED QT_ROOT)
    set(QT_ROOT "$ENV{QT_ROOT}")
endif()
if(QT_ROOT)
    list(APPEND CMAKE_PREFIX_PATH "${QT_ROOT}")
endif()

find_package(Qt6 ${LUMA_QT_VERSION} CONFIG REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

# Qt is LGPLv3; AGENTS.md §5 rule 7 requires dynamic linking only (no
# static Qt). This is Qt's own default for the open-source kits
# AGENTS.md's prerequisites install, not something to override here.
