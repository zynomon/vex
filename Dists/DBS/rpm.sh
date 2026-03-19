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
check_dep "rpmbuild" "rpm-build"

if [[ -z "${STAGE_DIR:-}" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
    source "${SCRIPT_DIR}/../CONF"
fi

mkdir -p "${PKG_OUT}"

WORK_DIR="${PKG_BUILD_BASE}/rpm"
RPMBUILD_ROOT="${WORK_DIR}/rpmbuild"
mkdir -p "${RPMBUILD_ROOT}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

SRC_TAR="${RPMBUILD_ROOT}/SOURCES/${PKG_NAME}_${PKG_VERSION}_${PKG_RELEASE}.tar.gz"
tar -czf "${SRC_TAR}" -C "${PROJECT_ROOT}" --exclude=build --exclude=dists/DBS/build .

SPEC_FILE="${RPMBUILD_ROOT}/SPECS/${PKG_NAME}.spec"
cat > "${SPEC_FILE}" <<EOF
Name:           ${PKG_NAME}
Version:        ${PKG_VERSION}
Release:        ${PKG_RELEASE}%{?dist}
Summary:        ${PKG_SUMMARY}
License:        ${PKG_LICENSE}
URL:            ${PKG_URL}
Requires:       ${RPM_DEPENDS}
BuildArch:      x86_64

%description
${PKG_SUMMARY}

%install
mkdir -p %{buildroot}/usr
cp -a ${STAGE_DIR}/usr/* %{buildroot}/usr/

%files
/usr/bin/${PKG_NAME}
/usr/lib/vex/*.so
/usr/share/applications/${PKG_NAME}.desktop
/usr/share/icons/hicolor/scalable/apps/${PKG_NAME}.svg

%postun

if [ \$1 -eq 0 ]; then
    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database >/dev/null 2>&1 || true
    fi
fi
EOF

rpmbuild \
    --define "_topdir ${RPMBUILD_ROOT}" \
    --define "_dbpath ${RPMBUILD_ROOT}/rpmdb" \
    -bb "${SPEC_FILE}"

find "${RPMBUILD_ROOT}/RPMS" -name "*.rpm" -exec bash -c '
    for file; do

        arch=$(basename "$file" | sed "s/.*\.\([^.]*\)\.rpm/\1/")

        newname="${PKG_OUT}/${PKG_NAME}_${PKG_VERSION}_${PKG_RELEASE}_${arch}.rpm"
        mv "$file" "$newname"
        echo "[RPM] Created: $(basename "$newname")"
    done
' bash {} +

echo "[RPM] Packages created in: ${PKG_OUT}"