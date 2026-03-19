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

mkdir -p "${PKG_OUT}"
mkdir -p "${TOOLS_DIR}"

if [ ! -f "${APPIMAGETOOL}" ]; then
    echo "appimagetool not found in ${TOOLS_DIR}. Downloading..."
    APPIMAGETOOL_URL="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
    wget -q --show-progress -O "${APPIMAGETOOL}" "${APPIMAGETOOL_URL}"
    chmod +x "${APPIMAGETOOL}"
else
    echo "Found appimagetool at ${APPIMAGETOOL}"
    chmod +x "${APPIMAGETOOL}"
fi

rm -rf "${APPDIR}"
mkdir -p "${APPDIR}"

IFS=', ' read -ra ALL_PACKAGES <<< "${DEB_DEPENDS}"

if [ ! -d "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu" ] || [ -z "$(ls -A "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu"/*.so* 2>/dev/null)" ]; then
    echo "Downloading all required libraries to ${DOWNLOAD_DIR}..."

    ARCH="amd64"
    TEMP_DEB_DIR="/tmp/pkg-debs-$$"
    mkdir -p "${TEMP_DEB_DIR}"
    mkdir -p "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu"
    mkdir -p "${DOWNLOAD_DIR}/usr/lib"
    mkdir -p "${DOWNLOAD_DIR}/lib"
    mkdir -p "${DOWNLOAD_DIR}/lib64"
    mkdir -p "${DOWNLOAD_DIR}/usr/share"

    if ! command -v apt-get &> /dev/null; then
        echo "[ERROR] apt-get not found. This script requires a Debian-based system."
        exit 1
    fi

    if ! command -v dpkg-deb &> /dev/null; then
        echo "[ERROR] dpkg-deb not found. This script requires a Debian-based system."
        exit 1
    fi

    cd "${TEMP_DEB_DIR}"

    for pkg in "${ALL_PACKAGES[@]}"; do
        pkg=$(echo "$pkg" | cut -d' ' -f1 | cut -d'(' -f1)
        echo "Processing ${pkg}..."

        if ! apt-get download -q "${pkg}"; then
            echo "  Warning: Failed to download ${pkg}"
            continue
        fi

        deb_file=$(find "${TEMP_DEB_DIR}" -name "${pkg}_*_${ARCH}.deb" | head -n1)

        if [ -z "$deb_file" ] || [ ! -f "$deb_file" ]; then
            echo "  Warning: Failed to find downloaded ${pkg}"
            continue
        fi

        echo "  Downloaded: $(basename "$deb_file")"

        extract_dir="${TEMP_DEB_DIR}/extract/${pkg}"
        mkdir -p "${extract_dir}"

        if ! dpkg-deb -x "$deb_file" "${extract_dir}"; then
            echo "  Warning: Failed to extract ${pkg}"
            continue
        fi

        echo "  Copying libraries from ${pkg}..."

        for libpath in \
            "usr/lib/x86_64-linux-gnu" \
            "usr/lib" \
            "lib/x86_64-linux-gnu" \
            "lib" \
            "lib64" \
            "usr/lib64"; do

            if [ -d "${extract_dir}/${libpath}" ]; then
                target_dir="${DOWNLOAD_DIR}/${libpath}"
                mkdir -p "${target_dir}"

                find "${extract_dir}/${libpath}" -name "*.so*" \( -type f -o -type l \) | while read libfile; do
                    cp -a "$libfile" "${target_dir}/"
                    echo "    Copied: $(basename "$libfile")"
                done
            fi
        done

        if [ -d "${extract_dir}/usr/share" ]; then

            for sharedir in "${extract_dir}/usr/share/"*; do
                if [ -d "$sharedir" ] && \
                   [[ "$(basename "$sharedir")" != "doc" ]] && \
                   [[ "$(basename "$sharedir")" != "man" ]] && \
                   [[ "$(basename "$sharedir")" != "info" ]]; then
                    target_dir="${DOWNLOAD_DIR}/usr/share/$(basename "$sharedir")"
                    mkdir -p "${target_dir}"
                    cp -a "$sharedir/." "${target_dir}/"
                    echo "    Copied share data: $(basename "$sharedir")"
                fi
            done
        fi

        if [ -d "${extract_dir}/etc" ]; then
            target_dir="${DOWNLOAD_DIR}/etc"
            mkdir -p "${target_dir}"
            cp -a "${extract_dir}/etc/." "${target_dir}/"
            echo "    Copied config files"
        fi

        echo "  Done with ${pkg}"
    done

    if [ -d "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu" ]; then
        cd "${DOWNLOAD_DIR}/usr/lib/x86_64-linux-gnu"
        echo "Creating library symlinks..."

        for lib in *.so.*; do
            if [[ -f "$lib" ]] && [[ "$lib" =~ \.so\.[0-9]+$ ]]; then
                base_name="${lib%%.so.*}.so"
                if [ ! -f "$base_name" ] && [ ! -L "$base_name" ]; then
                    ln -sf "$lib" "$base_name"
                    echo "  Created symlink: $base_name -> $lib"
                fi
            fi
        done
    fi

    for libdir in "${DOWNLOAD_DIR}"/usr/lib "${DOWNLOAD_DIR}"/lib "${DOWNLOAD_DIR}"/lib64; do
        if [ -d "$libdir" ]; then
            cd "$libdir"
            for lib in *.so.*; do
                if [[ -f "$lib" ]] && [[ "$lib" =~ \.so\.[0-9]+$ ]]; then
                    base_name="${lib%%.so.*}.so"
                    if [ ! -f "$base_name" ] && [ ! -L "$base_name" ]; then
                        ln -sf "$lib" "$base_name"
                        echo "  Created symlink in $(basename "$libdir"): $base_name -> $lib"
                    fi
                fi
            done
        fi
    done

    rm -rf "${TEMP_DEB_DIR}"
    echo "All required libraries downloaded and extracted to ${DOWNLOAD_DIR}"
fi

mkdir -p "${APPDIR}/usr/bin"
if [ -f "${STAGE_DIR}/usr/bin/${PKG_NAME}" ]; then
    cp -a "${STAGE_DIR}/usr/bin/${PKG_NAME}" "${APPDIR}/usr/bin/"

    ln -sf "usr/bin/${PKG_NAME}" "${APPDIR}/${PKG_NAME}"
else
    echo "[ERROR] Binary ${PKG_NAME} not found"
    exit 1
fi

if [ -d "${STAGE_DIR}/usr/lib/vex" ]; then
    mkdir -p "${APPDIR}/usr/lib/vex"
    cp -a "${STAGE_DIR}/usr/lib/vex/"*.so "${APPDIR}/usr/lib/vex/" 2>/dev/null || true
fi

echo "Copying cached libraries to AppDir..."
if [ -d "${DOWNLOAD_DIR}" ]; then

    for libdir in lib lib64 usr/lib usr/lib64 usr/lib/x86_64-linux-gnu; do
        if [ -d "${DOWNLOAD_DIR}/${libdir}" ]; then
            target_dir="${APPDIR}/${libdir}"
            mkdir -p "${target_dir}"
            cp -a "${DOWNLOAD_DIR}/${libdir}/"*.so* "${target_dir}/" 2>/dev/null || true
            echo "  Copied libraries from ${libdir}"
        fi
    done

    if [ -d "${DOWNLOAD_DIR}/usr/share" ]; then
        mkdir -p "${APPDIR}/usr/share"
        cp -a "${DOWNLOAD_DIR}/usr/share/." "${APPDIR}/usr/share/" 2>/dev/null || true
        echo "  Copied share data"
    fi
fi

mkdir -p "${APPDIR}/usr/share/applications"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/scalable/apps"

if [ -f "${STAGE_DIR}/usr/share/applications/${PKG_NAME}.desktop" ]; then
    cp -a "${STAGE_DIR}/usr/share/applications/${PKG_NAME}.desktop" "${APPDIR}/usr/share/applications/"
    cp "${APPDIR}/usr/share/applications/${PKG_NAME}.desktop" "${APPDIR}/"
fi

if [ -f "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" ]; then
    cp -a "${STAGE_DIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "${APPDIR}/usr/share/icons/hicolor/scalable/apps/"
    cp "${APPDIR}/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg" "${APPDIR}/"
fi

if [ -f "${APPDIR}/${PKG_NAME}.desktop" ]; then
    sed -i 's/^Version=.*/Version=1.0/' "${APPDIR}/${PKG_NAME}.desktop"
    if grep -q "^X-AppImage-Version=" "${APPDIR}/${PKG_NAME}.desktop"; then
        sed -i "s/^X-AppImage-Version=.*/X-AppImage-Version=${PKG_VERSION}/" "${APPDIR}/${PKG_NAME}.desktop"
    else
        echo "X-AppImage-Version=${PKG_VERSION}" >> "${APPDIR}/${PKG_NAME}.desktop"
    fi
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
"${APPIMAGETOOL}" "${APPDIR}" "${FINAL_FILE}"

if [ -f "${FINAL_FILE}" ]; then
    FILE_SIZE=$(stat -c%s "${FINAL_FILE}" 2>/dev/null || stat -f%z "${FINAL_FILE}" 2>/dev/null)
    if [ "${FILE_SIZE}" -gt 100000 ]; then
        echo "[SUCCESS] $(basename "${FINAL_FILE}") ($(du -sh "${FINAL_FILE}" | awk '{print $1}'))"
    else
        echo "[ERROR] AppImage too small (${FILE_SIZE} bytes)."
        exit 1
    fi
else
    echo "[ERROR] AppImage not created: ${FINAL_FILE}"
    exit 1
fi