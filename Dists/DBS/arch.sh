#!/usr/bin/env bash
set -euo pipefail

check_dep() {
    local cmd="$1" pkg="$2"
    command -v "$cmd" >/dev/null 2>&1 && return 0
    echo "[DEP] $cmd not found, attempting to install $pkg..."
    if command -v pacman >/dev/null 2>&1; then
        sudo pacman -S --noconfirm "$pkg"
    elif command -v apt-get >/dev/null 2>&1; then
        echo "[WARN] On Debian/Ubuntu - arch packages cannot be built here"
        return 1
    else
        echo "[ERROR] Cannot install $pkg: no supported package manager"
        exit 1
    fi
}
check_dep "makepkg" "pacman"

if [[ -z "${STAGE_DIR:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    source "${SCRIPT_DIR}/../CONF"
fi

mkdir -p "${PKG_OUT}"

WORK_DIR="${PKG_BUILD_BASE}/arch"
mkdir -p "${WORK_DIR}"

PKG_ROOT="${WORK_DIR}/pkg"
mkdir -p "${PKG_ROOT}"

mkdir -p "${PKG_ROOT}/usr/bin"
mkdir -p "${PKG_ROOT}/usr/lib/vex"
mkdir -p "${PKG_ROOT}/usr/share/applications"
mkdir -p "${PKG_ROOT}/usr/share/icons/hicolor/scalable/apps"

cp -a "${STAGE_DIR}/usr/bin/${PKG_NAME}" "${PKG_ROOT}/usr/bin/" 2>/dev/null || true
cp -a "${STAGE_DIR}/usr/lib/vex/"*.so "${PKG_ROOT}/usr/lib/vex/" 2>/dev/null || true
cp -a "${STAGE_DIR}/usr/share/applications/${PKG_NAME}.desktop" "${PKG_ROOT}/usr/share/applications/" 2>/dev/null || true
cp -a "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "${PKG_ROOT}/usr/share/icons/hicolor/scalable/apps/" 2>/dev/null || true

cat > "${WORK_DIR}/PKGBUILD" <<EOF
pkgname=${PKG_NAME}
pkgver=${PKG_VERSION}
pkgrel=${PKG_RELEASE}
pkgdesc="${PKG_SUMMARY}"
arch=('x86_64')
url="${PKG_URL}"
license=('${PKG_LICENSE}')
depends=(${ARCH_DEPENDS})
provides=('${PKG_NAME}')
conflicts=('${PKG_NAME}')

package() {
    install -Dm755 "${PKG_ROOT}/usr/bin/${PKG_NAME}" "\${pkgdir}/usr/bin/${PKG_NAME}"
    
    install -Dm644 "${PKG_ROOT}/usr/lib/vex/VexCore.so" "\${pkgdir}/usr/lib/vex/VexCore.so"
    install -Dm644 "${PKG_ROOT}/usr/lib/vex/SyntaxCore.so" "\${pkgdir}/usr/lib/vex/SyntaxCore.so"
    install -Dm644 "${PKG_ROOT}/usr/lib/vex/LookAndFeelCore.so" "\${pkgdir}/usr/lib/vex/LookAndFeelCore.so"
    
    install -Dm644 "${PKG_ROOT}/usr/share/applications/${PKG_NAME}.desktop" "\${pkgdir}/usr/share/applications/${PKG_NAME}.desktop"
    install -Dm644 "${PKG_ROOT}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "\${pkgdir}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg"
}

post_install() {
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database >/dev/null 2>&1 || true
    fi
}

post_upgrade() {
    post_install
}

post_remove() {
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database >/dev/null 2>&1 || true
    fi
}
EOF

if [[ -f "${DBS_DIR}/../linux/arch/onu.install" ]]; then
    cp "${DBS_DIR}/../linux/arch/onu.install" "${WORK_DIR}/${PKG_NAME}.install"
    echo "install=\${pkgname}.install" >> "${WORK_DIR}/PKGBUILD"
fi

cd "${WORK_DIR}"

makepkg --cleanbuild --skipinteg --noconfirm

find . -name "*.pkg.tar.zst" -exec bash -c '
    for file; do
        newname="${PKG_OUT}/${PKG_NAME}_${PKG_VERSION}_${PKG_RELEASE}_x86_64.pkg.tar.zst"
        mv "$file" "$newname"
        echo "[ARCH] Created: $(basename "$newname")"
    done
' bash {} +

echo "[ARCH] Packages created in: ${PKG_OUT}"
