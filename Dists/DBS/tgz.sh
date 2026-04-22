#!/usr/bin/env bash
set -e

FOLDER_NAME="${PKG_NAME}-${PKG_VERSION}-${PKG_RELEASE}"
WORKSPACE="${PKG_BUILD_BASE}/tgz/${FOLDER_NAME}"

echo "[PKG-TGZ] Creating workspace: ${WORKSPACE}"
rm -rf "${WORKSPACE}"
mkdir -p "${WORKSPACE}"

cp -a "${STAGE_DIR}/usr/." "${WORKSPACE}/" 2>/dev/null || true

cat << EOF > "${WORKSPACE}/README.txt"
 ${PKG_NAME^^} v${PKG_VERSION} (${BUILD_TYPE})
_______________________________________________________________________________/

FILES:
  vex        - binary
  *.so       - libraries (VexCore.so, SyntaxCore.so, LookAndFeelCore.so)
  *.svg      - icon
  *.desktop  - desktop entry

INSTALLATION:
_______________________________________
Run: sh install.sh

Manual System-wide:
  su -c 'cp vex /bin/'
  su -c 'mkdir -p /lib/vex'
  su -c 'cp *.so /lib/vex/'
  su -c 'cp *.svg /usr/share/icons/hicolor/scalable/apps/'
  su -c 'cp *.desktop /usr/share/applications/'

Manual User-only (~/.local):
  cp vex ~/.local/bin/
  mkdir -p ~/.local/lib/vex
  cp *.so ~/.local/lib/vex/
  cp *.svg ~/.local/share/icons/
  cp *.desktop ~/.local/share/applications/

for removing use 
sh remove.sh


DEPENDENCIES:
_______________________________________
Debian/Ubuntu: ${DEB_DEPENDS}
Fedora/RHEL:   ${RPM_DEPENDS}
Arch:          ${ARCH_DEPENDS}

Built: $(date)
EOF

cat << 'SCRIPT' > "${WORKSPACE}/install.sh"
#!/bin/sh
set -e
green='\033[0;32m'
white='\033[1;37m'
nc='\033[0m'

echo "${green}V${white}ex${nc}"
echo "_______________________________________"
printf "Install system-wide? [y/N]: "
read answer

case "$answer" in
    [Yy]|[Yy][Ee][Ss])
        echo "${white}[INSTALL] System-wide <requires root>"
        su -c 'cp vex /bin/'
        su -c 'mkdir -p /lib/vex'
        su -c 'cp *.so /lib/vex/'
        su -c 'cp *.svg /usr/share/icons/hicolor/scalable/apps/ 2>/dev/null || cp *.svg /usr/share/icons/'
        su -c 'cp *.desktop /usr/share/applications/'
        ;;
    *)
        echo "${white}[INSTALL] User-only (~/.local)"
        mkdir -p ~/.local/bin
        cp vex ~/.local/bin/
        mkdir -p ~/.local/lib/vex
        cp *.so ~/.local/lib/vex/
        mkdir -p ~/.local/share/icons
        cp *.svg ~/.local/share/icons/
        mkdir -p ~/.local/share/applications
        cp *.desktop ~/.local/share/applications/
        echo "${nc}Note: Add ~/.local/bin to PATH if not already"
        ;;
esac

echo "[DONE] Installation complete"
SCRIPT

cat << 'SCRIPT' > "${WORKSPACE}/remove.sh"
#!/bin/sh
set -e

echo "'\033[0;32mV\033[1;37mex Remover"
echo "'\033[0;32m_______________________________________"

if [ -f /bin/vex ]; then
    echo "[REMOVE] System-wide installation"
    su -c 'rm -f /bin/vex'
    su -c 'rm -rf /lib/vex'
    su -c 'rm -f /usr/share/icons/hicolor/scalable/apps/vex.svg'
    su -c 'rm -f /usr/share/icons/vex.svg'
    su -c 'rm -f /usr/share/applications/vex.desktop'
fi

if [ -f ~/.local/bin/vex ]; then
    echo "[REMOVE] User installation"
    rm -f ~/.local/bin/vex
    rm -rf ~/.local/lib/vex
    rm -f ~/.local/share/icons/vex.svg
    rm -f ~/.local/share/applications/vex.desktop
fi

rm -rf /home/*/.vex  
rm -rf /root/.vex

echo "[DONE] Removal complete"
SCRIPT

chmod +x "${WORKSPACE}/install.sh" "${WORKSPACE}/remove.sh"

FINAL_FILE="${PKG_OUT}/${FOLDER_NAME}.tar.zst"
echo "[PKG-TGZ] Compressing to ${FINAL_FILE} (Zstd -22)..."

cd "${PKG_BUILD_BASE}/tgz"

if command -v zstd >/dev/null 2>&1; then
    tar cf - "${FOLDER_NAME}" | zstd -22 --ultra -o "${FINAL_FILE}"
else
    echo "[WARN] zstd not found, falling back to gzip -9"
    tar -czf "${PKG_OUT}/${FOLDER_NAME}.tar.gz" "${FOLDER_NAME}"
fi

if [ -f "${FINAL_FILE}" ]; then
    echo "[SUCCESS] Tarball created: $(basename "${FINAL_FILE}")"
    echo "[SIZE] $(du -sh "${FINAL_FILE}" | awk '{print $1}')"
fi