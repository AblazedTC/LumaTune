#!/usr/bin/env bash
# PLAN.md P2.8 / §12's Linux row: linuxdeploy + linuxdeploy-plugin-qt ->
# AppImage (linuxdeployqt is unmaintained - deliberately not used), plus
# a plain tarball of the same AppDir. Unsigned (P2.8's own acceptance);
# AppImage signing is P7.1's job, later. Tool releases are pinned, not
# "continuous"/"latest", for reproducibility.
set -euo pipefail

BUILD_DIR="${1:?usage: package.sh <build-dir> <output-dir>}"
OUT_DIR="${2:?usage: package.sh <build-dir> <output-dir>}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

LINUXDEPLOY_VERSION="1-alpha-20250213-1"
# linuxdeploy-plugin-qt has no versioned releases as of this writing -
# "continuous" is the only tag it publishes.
LINUXDEPLOY_QT_VERSION="continuous"
VERSION="0.1.0" # mirrors main.cpp's kVersion (P2.1) - no single source of truth yet

TOOLS_DIR="${BUILD_DIR}/packaging-tools"
mkdir -p "${TOOLS_DIR}" "${OUT_DIR}"

download_tool() {
    local url="$1" dest="$2"
    if [ ! -x "${dest}" ]; then
        curl -fL "${url}" -o "${dest}"
        chmod +x "${dest}"
    fi
}

download_tool \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/${LINUXDEPLOY_VERSION}/linuxdeploy-x86_64.AppImage" \
    "${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
download_tool \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/${LINUXDEPLOY_QT_VERSION}/linuxdeploy-plugin-qt-x86_64.AppImage" \
    "${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"

APPDIR="${BUILD_DIR}/AppDir"
rm -rf "${APPDIR}"

export PATH="${TOOLS_DIR}:${PATH}"
# Our own QML is compiled into the binary via qt_add_qml_module's
# resource system, not loose files - but linuxdeploy-plugin-qt's
# qmlimportscanner still needs *some* .qml text to scan in order to
# know which built-in Qt Quick modules (Controls, Dialogs, ...) to
# bundle, and the source files are still on disk here even though
# they're also embedded in the binary.
export QML_SOURCES_PATHS="${REPO_ROOT}/src/ui/qml"
# GitHub-hosted runners generally lack FUSE, which a plain AppImage
# needs to mount itself - this makes linuxdeploy (and the AppImages it
# produces) extract-and-run instead, no FUSE required.
export APPIMAGE_EXTRACT_AND_RUN=1

"${TOOLS_DIR}/linuxdeploy-x86_64.AppImage" \
    --appdir "${APPDIR}" \
    -e "${BUILD_DIR}/src/app/lumatune" \
    -d "${REPO_ROOT}/src/app/resources/lumatune.desktop" \
    -i "${REPO_ROOT}/src/app/resources/lumatune.png" \
    --plugin qt \
    --output appimage

mv ./LumaTune*.AppImage "${OUT_DIR}/LumaTune-${VERSION}-x86_64.AppImage"

# "Also publish a tarball" (PLAN.md §12) - the AppDir linuxdeploy just
# built is already a complete, relocatable bundle.
tar -C "${BUILD_DIR}" -czf "${OUT_DIR}/LumaTune-${VERSION}-linux-x86_64.tar.gz" AppDir
