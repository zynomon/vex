#!/usr/bin/env bash
set -euo pipefail

check_dep() {
    local cmd="$1" pkg="$2"
    command -v "$cmd" >/dev/null 2>&1 && return 0
    echo "[DEP] $cmd not found, attempting to install $pkg..."
    if command -v apt-get >/dev/null 2>&1; then
        sudo apt-get update && sudo apt-get install -y "$pkg"
    elif command -v dnf >/dev/null 2>&1; then
        sudo dnf install -y "$pkg"
    elif command -v pacman >/dev/null 2>&1; then
        sudo pacman -S --noconfirm "$pkg"
    else
        echo "[ERROR] Cannot install $pkg: no supported package manager"
        exit 1
    fi
}
check_dep "dpkg-deb" "dpkg-dev"

if [[ -z "${STAGE_DIR:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    source "${SCRIPT_DIR}/../CONF"
fi

mkdir -p "${PKG_OUT}"

WORK_DIR="${PKG_BUILD_BASE}/deb"
mkdir -p "${WORK_DIR}"
CTRL_DIR="${WORK_DIR}/DEBIAN"
mkdir -p "${CTRL_DIR}"

mkdir -p "${WORK_DIR}/usr"
cp -a "${STAGE_DIR}/usr/." "${WORK_DIR}/usr/"

ARCH=$(dpkg --print-architecture 2>/dev/null || echo "amd64")

cat > "${CTRL_DIR}/control" <<EOF
Package: ${PKG_NAME}
Version: ${PKG_VERSION}-${PKG_RELEASE}
Section: utils
Priority: optional
Architecture: ${ARCH}
Maintainer: ${PKG_MAINTAINER}
Description: ${PKG_SUMMARY}
Depends: ${DEB_DEPENDS}
EOF

[[ -f "${DBS_DIR}/../linux/deb/postrm" ]] && cp "${DBS_DIR}/../linux/deb/postrm" "${CTRL_DIR}/" && chmod 755 "${CTRL_DIR}/postrm"

PKG_FILE="${PKG_OUT}/${PKG_NAME}_${PKG_VERSION}_${PKG_RELEASE}_${ARCH}.deb"
dpkg-deb --root-owner-group --build "${WORK_DIR}" "${PKG_FILE}"

echo "[DEB] Created: $(basename "${PKG_FILE}")"