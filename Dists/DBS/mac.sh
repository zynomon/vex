#!/usr/bin/env bash
set -euo pipefail

check_dep() {
    local cmd="$1" pkg="$2"
    command -v "$cmd" >/dev/null 2>&1 && return 0
    echo "[DEP] $cmd not found: $pkg"
    return 1
}
check_dep "hdiutil" "macOS required" || exit 1

if [[ -z "${STAGE_DIR:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    source "${SCRIPT_DIR}/../CONF"
fi

mkdir -p "${PKG_OUT}"

WORK_DIR="${PKG_BUILD_BASE}/mac"
APP_BUNDLE="${WORK_DIR}/${PKG_NAME}.app"

if [ -d "${BUILD_DIR}/vex.app" ]; then
    cp -a "${BUILD_DIR}/vex.app" "${APP_BUNDLE}"
elif [ -d "${STAGE_DIR}/vex.app" ]; then
    cp -a "${STAGE_DIR}/vex.app" "${APP_BUNDLE}"
else
    echo "[ERROR] vex.app not found in build or stage directory"
    exit 1
fi

DMG_FILE="${PKG_OUT}/${PKG_NAME}_${PKG_VERSION}_${PKG_RELEASE}_macos.dmg"
hdiutil create -volname "${PKG_NAME} ${PKG_VERSION}" \
               -srcfolder "${APP_BUNDLE}" \
               -ov -format UDZO \
               "${DMG_FILE}"

echo "[MAC] DMG created: $(basename "${DMG_FILE}")"
