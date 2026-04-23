#!/usr/bin/env bash
set -e

: "${PROJECT_ROOT:?PROJECT_ROOT not set}"
: "${PKG_BUILD_BASE:?PKG_BUILD_BASE not set}"
: "${STAGE_DIR:?STAGE_DIR not set}"
: "${PKG_VERSION:?PKG_VERSION not set}"
: "${PKG_RELEASE:?PKG_RELEASE not set}"
: "${PKG_NAME:?PKG_NAME not set}"
: "${PKG_OUT:?PKG_OUT not set}"
: "${DEB_DEPENDS:?DEB_DEPENDS not set}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_DIR="${SCRIPT_DIR}/../tools"
APPIMAGETOOL="${TOOLS_DIR}/appimagetool"
APPDIR="${PKG_BUILD_BASE}/AppDir"
DOWNLOAD_DIR="${PROJECT_ROOT}/Dists/linux/appimage"

mkdir -pv "${PKG_OUT}" "${TOOLS_DIR}"

if [ ! -f "${APPIMAGETOOL}" ]; then
    echo "Downloading appimagetool..."
    wget -q --show-progress -O "${APPIMAGETOOL}" "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
    chmod -v +x "${APPIMAGETOOL}"
fi

rm -rf "${APPDIR}"
mkdir -pv "${APPDIR}"

IFS=', ' read -ra ALL_PACKAGES <<< "${DEB_DEPENDS}"

if [ ! -d "${DOWNLOAD_DIR}" ] || [ -z "$(ls -A "${DOWNLOAD_DIR}" 2>/dev/null)" ]; then
    echo "Downloading dependencies to ${DOWNLOAD_DIR}..."
    TEMP_DEB_DIR="/tmp/pkg-debs-$$"
    mkdir -pv "${TEMP_DEB_DIR}"
    mkdir -pv "${DOWNLOAD_DIR}"

    cd "${TEMP_DEB_DIR}"

    for pkg in "${ALL_PACKAGES[@]}"; do
        pkg=$(echo "$pkg" | cut -d' ' -f1 | cut -d'(' -f1)
        [ -z "$pkg" ] && continue
        echo "Downloading ${pkg}..."
        apt-get download -q "${pkg}" || continue
        
        deb_file=$(find . -name "${pkg}_*.deb" | head -1)
        [ -z "$deb_file" ] && continue

        dpkg-deb -x "$deb_file" "${DOWNLOAD_DIR}"
    done

    rm -rf "${TEMP_DEB_DIR}"
fi

mkdir -pv "${APPDIR}/usr/bin"
cp -av "${STAGE_DIR}/usr/bin/${PKG_NAME}" "${APPDIR}/usr/bin/"
ln -sfv "usr/bin/${PKG_NAME}" "${APPDIR}/${PKG_NAME}"

if [ -d "${STAGE_DIR}/usr/lib/vex" ]; then
    mkdir -pv "${APPDIR}/usr/lib/vex"
    cp -av "${STAGE_DIR}/usr/lib/vex/"*.so "${APPDIR}/usr/lib/vex/"
fi

if [ -d "${DOWNLOAD_DIR}/usr" ]; then
    cp -av "${DOWNLOAD_DIR}/usr/." "${APPDIR}/usr/"
fi
if [ -d "${DOWNLOAD_DIR}/lib" ]; then
    cp -av "${DOWNLOAD_DIR}/lib/." "${APPDIR}/lib/"
fi
if [ -d "${DOWNLOAD_DIR}/lib64" ]; then
    cp -av "${DOWNLOAD_DIR}/lib64/." "${APPDIR}/lib64/"
fi

mkdir -pv "${APPDIR}/usr/share/applications" "${APPDIR}/usr/share/icons/hicolor/scalable/apps"

if [ -f "${STAGE_DIR}/usr/share/applications/${PKG_NAME}.desktop" ]; then
    cp -av "${STAGE_DIR}/usr/share/applications/${PKG_NAME}.desktop" "${APPDIR}/usr/share/applications/"
    cp -av "${APPDIR}/usr/share/applications/${PKG_NAME}.desktop" "${APPDIR}/"
    
    sed -i 's/^Version=.*/Version=1.0/' "${APPDIR}/${PKG_NAME}.desktop"
    sed -i 's/^Categories=.*/Categories=Utility;Development;TextEditor;Qt;/' "${APPDIR}/${PKG_NAME}.desktop"
    sed -i 's/^Icon=.*/Icon=vex/' "${APPDIR}/${PKG_NAME}.desktop"
    echo "X-AppImage-Version=${PKG_VERSION}" >> "${APPDIR}/${PKG_NAME}.desktop"
fi

if [ -f "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" ]; then
    cp -av "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "${APPDIR}/usr/share/icons/hicolor/scalable/apps/"
    cp -av "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "${APPDIR}/${PKG_NAME}.svg"
fi

if [ "${PKG_NAME}" != "vex" ] && [ -f "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/vex.svg" ]; then
    cp -av "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/vex.svg" "${APPDIR}/usr/share/icons/hicolor/scalable/apps/"
    cp -av "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/vex.svg" "${APPDIR}/vex.svg"
fi

cat > "${APPDIR}/AppRun" << EOF
#!/bin/bash
HERE="\$(dirname "\$(readlink -f "\$0")")"
export LD_LIBRARY_PATH="\${HERE}/usr/lib:\${HERE}/usr/lib/x86_64-linux-gnu:\${HERE}/lib:\${HERE}/lib64:\${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="\${HERE}/usr/lib/x86_64-linux-gnu/qt6/plugins"
exec "\${HERE}/usr/bin/${PKG_NAME}" "\$@"
EOF
chmod -v +x "${APPDIR}/AppRun"

FINAL_FILE="${PKG_OUT}/${PKG_NAME}_${PKG_VERSION}_${PKG_RELEASE}_x86_64.AppImage"
echo "Creating AppImage..."

if ! "${APPIMAGETOOL}" --version 2>/dev/null; then
    "${APPIMAGETOOL}" --appimage-extract
    ./squashfs-root/AppRun "${APPDIR}" "${FINAL_FILE}"
    rm -rf squashfs-root
else
    "${APPIMAGETOOL}" "${APPDIR}" "${FINAL_FILE}"
fi

if [ -f "${FINAL_FILE}" ]; then
    echo "[SUCCESS] $(basename "${FINAL_FILE}") ($(du -sh "${FINAL_FILE}" | awk '{print $1}'))"
else
    echo "[ERROR] AppImage not created"
    exit 1
fi
