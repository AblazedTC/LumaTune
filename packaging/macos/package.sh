#!/usr/bin/env bash
# PLAN.md P2.8 / §12's macOS row: macdeployqt LumaTune.app -qmldir=...
# -dmg. Unsigned (P2.8's own acceptance) - Developer ID signing,
# Hardened Runtime, and notarytool submission are all §12's documented
# "Mandatory" requirements for real distribution, but that's P7.1's
# job, later; an unsigned CI artefact never gets Gatekeeper's
# quarantine attribute in the first place (that's only added to files
# received via a browser/Mail/etc, never to something built and run
# on the same machine), so none of that blocks this chunk's own
# acceptance ("Artefacts launch on clean VMs").
set -euo pipefail

BUILD_DIR="${1:?usage: package.sh <build-dir> <output-dir>}"
OUT_DIR="${2:?usage: package.sh <build-dir> <output-dir>}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

VERSION="0.1.0" # mirrors main.cpp's kVersion (P2.1) - no single source of truth yet
APP_BUNDLE="${BUILD_DIR}/src/app/lumatune.app"

mkdir -p "${OUT_DIR}"

# qt_add_executable (src/app/CMakeLists.txt) sets MACOSX_BUNDLE TRUE by
# default on Apple platforms once a target links Qt::Quick
# (transitively, via luma_ui) - lumatune is already a real .app
# bundle, nothing extra needed to produce one here.
macdeployqt "${APP_BUNDLE}" "-qmldir=${REPO_ROOT}/src/ui/qml" -dmg

mv "${BUILD_DIR}/src/app/lumatune.dmg" "${OUT_DIR}/LumaTune-${VERSION}-macos-arm64.dmg"
