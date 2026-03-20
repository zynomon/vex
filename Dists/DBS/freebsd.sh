#!/usr/bin/env bash
set -euo pipefail

check_dep() {
    local cmd="$1" pkg="$2"
    command -v "$cmd" >/dev/null 2>&1 && return 0
    echo "[DEP] $cmd not found: $pkg"
    return 1
}
check_dep "pkg" "FreeBSD required" || exit 1

if [[ -z "${STAGE_DIR:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    source "${SCRIPT_DIR}/../CONF"
fi

mkdir -p "${PKG_OUT}"

WORK_DIR="${PKG_BUILD_BASE}/bsd"
PKG_ROOT="${WORK_DIR}/stage"
rm -rf "${PKG_ROOT}"
mkdir -p "${PKG_ROOT}"

mkdir -p "${PKG_ROOT}/usr/bin"
mkdir -p "${PKG_ROOT}/usr/lib/vex"
mkdir -p "${PKG_ROOT}/usr/share/applications"
mkdir -p "${PKG_ROOT}/usr/share/icons/hicolor/scalable/apps"

cp -a "${STAGE_DIR}/usr/bin/${PKG_NAME}" "${PKG_ROOT}/usr/bin/" 2>/dev/null || true
cp -a "${STAGE_DIR}/usr/lib/vex/"*.so "${PKG_ROOT}/usr/lib/vex/" 2>/dev/null || true
cp -a "${STAGE_DIR}/usr/share/applications/${PKG_NAME}.desktop" "${PKG_ROOT}/usr/share/applications/" 2>/dev/null || true
cp -a "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "${PKG_ROOT}/usr/share/icons/hicolor/scalable/apps/" 2>/dev/null || true
cat > "${PKG_ROOT}/+MANIFEST" <<EOF
name: ${PKG_NAME}
version: ${PKG_VERSION}_${PKG_RELEASE}
origin: editors/${PKG_NAME}
comment: ${PKG_SUMMARY}
maintainer: ${PKG_MAINTAINER}
www: ${PKG_URL}
prefix: /usr
licenselogic: single
licenses: [APACHE20]
desc: |-
  ${PKG_SUMMARY}
  
  Vex is an Extensive Qt C++ text editor with syntax highlighting
  and plugin support.
deps: {
 qt6-base: { origin "qt6/qt6-base" }
}
EOF

cat > "${PKG_ROOT}/+INSTALL" <<'EOF'
#!/bin/sh

echo "\nThanks for trying Vex\n"

EOF
chmod +x "${PKG_ROOT}/+INSTALL"

cat > "${PKG_ROOT}/+DEINSTALL" <<'EOF'
#!/bin/sh
case $2 in
  POST-DEINSTALL)
    if command -v update-desktop-database >/dev/null 2>&1; then
      update-desktop-database >/dev/null 2>&1 || true
    fi
    ;;
esac

rm -rf /usr/lib/vex
rm -rf /usr/bin/vex
rm -ri /home/*/.vex/
EOF
chmod +x "${PKG_ROOT}/+DEINSTALL"

cd "${PKG_ROOT}"
PKG_FILE="${PKG_OUT}/${PKG_NAME}_${PKG_VERSION}_${PKG_RELEASE}.pkg"
pkg create -r . -o "${PKG_OUT}" -m .

find "${PKG_OUT}" -name "*.pkg" -not -name "*.pkg" -exec bash -c '
    for file; do
        newname="${PKG_OUT}/${PKG_NAME}_${PKG_VERSION}_${PKG_RELEASE}.pkg"
        mv "$file" "$newname"
        echo "[FREEBSD] Created: $(basename "$newname")"
    done
' bash {} +

echo "[FREEBSD] Package created in: ${PKG_OUT}"
