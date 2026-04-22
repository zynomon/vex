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

mkdir -p "${PKG_OUT}" "${TOOLS_DIR}"

if [ ! -f "${APPIMAGETOOL}" ]; then
    echo "Downloading appimagetool..."
    wget -q --show-progress -O "${APPIMAGETOOL}" "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
    chmod +x "${APPIMAGETOOL}"
fi

rm -rf "${APPDIR}"
mkdir -p "${APPDIR}"

IFS=', ' read -ra ALL_PACKAGES <<< "${DEB_DEPENDS}"

if [ ! -d "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu" ] || [ -z "$(ls -A "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu"/*.so* 2>/dev/null)" ]; then
    echo "Downloading libraries to ${DOWNLOAD_DIR}..."

    TEMP_DEB_DIR="/tmp/pkg-debs-$$"
    mkdir -p "${TEMP_DEB_DIR}"
    mkdir -p "${DOWNLOAD_DIR}"/{usr/lib/x86_64-linux-gnu,usr/lib,lib,lib64,usr/share}

    cd "${TEMP_DEB_DIR}"

    for pkg in "${ALL_PACKAGES[@]}"; do
        pkg=$(echo "$pkg" | cut -d' ' -f1 | cut -d'(' -f1)
        echo "Processing ${pkg}..."

        if ! apt-get download -q "${pkg}" 2>/dev/null; then
            echo "  Failed to download ${pkg}, checking system..."
            if ls /usr/lib/x86_64-linux-gnu/${pkg}.so* 2>/dev/null; then
                echo "  Using system ${pkg}"
                mkdir -p "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu"
                cp -a /usr/lib/x86_64-linux-gnu/${pkg}.so* "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu/" 2>/dev/null || true
            fi
            continue
        fi

        deb_file=$(find . -name "${pkg}_*.deb" | head -1)
        [ -z "$deb_file" ] && continue

        extract_dir="${TEMP_DEB_DIR}/extract/${pkg}"
        mkdir -p "${extract_dir}"
        dpkg-deb -x "$deb_file" "${extract_dir}"

        for libpath in "usr/lib/x86_64-linux-gnu" "usr/lib" "lib/x86_64-linux-gnu" "lib" "lib64"; do
            if [ -d "${extract_dir}/${libpath}" ]; then
                target_dir="${DOWNLOAD_DIR}/${libpath}"
                mkdir -p "${target_dir}"
                find "${extract_dir}/${libpath}" -name "*.so*" -exec cp -a {} "${target_dir}/" \; 2>/dev/null || true
            fi
        done
    done

    cd "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu" 2>/dev/null && for lib in *.so.*; do
        [ -f "$lib" ] && ln -sf "$lib" "${lib%%.so.*}.so" 2>/dev/null || true
    done

    rm -rf "${TEMP_DEB_DIR}"
fi

mkdir -p "${APPDIR}/usr/bin"
cp -a "${STAGE_DIR}/usr/bin/${PKG_NAME}" "${APPDIR}/usr/bin/"
ln -sf "usr/bin/${PKG_NAME}" "${APPDIR}/${PKG_NAME}"

if [ -d "${STAGE_DIR}/usr/lib/vex" ]; then
    mkdir -p "${APPDIR}/usr/lib/vex"
    cp -a "${STAGE_DIR}/usr/lib/vex/"*.so "${APPDIR}/usr/lib/vex/" 2>/dev/null || true
fi

for libdir in lib lib64 usr/lib usr/lib64 usr/lib/x86_64-linux-gnu; do
    [ -d "${DOWNLOAD_DIR}/${libdir}" ] && cp -a "${DOWNLOAD_DIR}/${libdir}/"*.so* "${APPDIR}/${libdir}/" 2>/dev/null || true
done

mkdir -p "${APPDIR}/usr/share/applications" "${APPDIR}/usr/share/icons/hicolor/scalable/apps"

if [ -f "${STAGE_DIR}/usr/share/applications/${PKG_NAME}.desktop" ]; then
    cp -a "${STAGE_DIR}/usr/share/applications/${PKG_NAME}.desktop" "${APPDIR}/usr/share/applications/"
    cp "${APPDIR}/usr/share/applications/${PKG_NAME}.desktop" "${APPDIR}/"
    
    sed -i 's/^Version=.*/Version=1.0/' "${APPDIR}/${PKG_NAME}.desktop"
    sed -i 's/^Categories=.*/Categories=Utility;Development;TextEditor;Qt;/' "${APPDIR}/${PKG_NAME}.desktop"
    sed -i 's/^Icon=.*/Icon=vex/' "${APPDIR}/${PKG_NAME}.desktop"
    echo "X-AppImage-Version=${PKG_VERSION}" >> "${APPDIR}/${PKG_NAME}.desktop"
fi

if [ -f "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" ]; then
    cp -a "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "${APPDIR}/usr/share/icons/hicolor/scalable/apps/"
    cp "${APPDIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "${APPDIR}/${PKG_NAME}.svg"
fi

cat > "${APPDIR}/AppRun" << EOF
#!/bin/bash
HERE="\$(dirname "\$(readlink -f "\$0")")"
export LD_LIBRARY_PATH="\${HERE}/usr/lib:\${HERE}/usr/lib/x86_64-linux-gnu:\${HERE}/lib:\${HERE}/lib64:\${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="\${HERE}/usr/lib/qt6/plugins:\${HERE}/usr/lib/x86_64-linux-gnu/qt6/plugins"
exec "\${HERE}/usr/bin/${PKG_NAME}" "\$@"
EOF
chmod +x "${APPDIR}/AppRun"

FINAL_FILE="${PKG_OUT}/${PKG_NAME}_${PKG_VERSION}_${PKG_RELEASE}_x86_64.AppImage"
echo "Creating AppImage..."

if ! "${APPIMAGETOOL}" --version &>/dev/null; then
    "${APPIMAGETOOL}" --appimage-extract >/dev/null 2>&1
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