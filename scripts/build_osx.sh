#!/bin/bash
# Build, codesign, notarize, and package reaper_libsndfilewrapper for macOS.
#
# Usage:  ./scripts/build_osx.sh [--no-sign]
#
# Output: signed + notarized .pkg installer in _OSX_RELEASE/ that drops
#   /Library/Application Support/REAPER/UserPlugins/reaper_libsndfilewrapper.dylib
#         (statically linked: libsndfile + WDL — no external deps)
#
# Because everything (libsndfile, WDL) is statically linked, there are no
# runtime dylibs to bundle or relink — the pipeline is just:
#   1. cmake configure (INSTALL_USER_PLUGINS=OFF) + universal build
#   2. codesign the plugin with Developer ID Application + hardened runtime
#   3. pkgbuild the staging tree as a flat installer
#   4. productsign with Developer ID Installer
#   5. notarytool submit (--wait) + stapler staple
#   6. spctl assess for sanity

set -e

ROOT=$(cd "$(dirname "$0")/.."; pwd)
BUILD_DIR="$ROOT/_build"
STAGE_DIR="$BUILD_DIR/stage"
PLUGIN_NAME="reaper_libsndfilewrapper.dylib"
INSTALL_PARENT="/Library/Application Support/REAPER/UserPlugins"
LICENSE_SUBDIR="reaper_libsndfilewrapper-licenses"
VERSION=$(<"$ROOT/VERSION")

# Parse arguments
SKIP_SIGN=false
while [[ $# -gt 0 ]]; do
    case "$1" in
        --no-sign) SKIP_SIGN=true ;;
        *) echo "Unknown option: $1"; echo "Usage: $0 [--no-sign]"; exit 1 ;;
    esac
    shift
done

# Load codesigning credentials (skip if --no-sign)
if ! $SKIP_SIGN; then
    CODESIGN_ENV="$ROOT/scripts/codesign.env"
    if [ ! -f "$CODESIGN_ENV" ]; then
        echo "Error: $CODESIGN_ENV not found. Create it with your codesigning credentials."
        exit 1
    fi
    # shellcheck disable=SC1090
    source "$CODESIGN_ENV"
fi

codesign_one() {
    codesign -s "$CODESIGN_APP" \
             --force --strict --timestamp --options=runtime \
             "$1"
}

# =========================================================
# Clean & prepare
# =========================================================

echo ""; echo "=== reaper_libsndfilewrapper v$VERSION — macOS installer build ==="

rm -rf "$BUILD_DIR" 2>/dev/null || sudo rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
mkdir -p "$STAGE_DIR$INSTALL_PARENT"
mkdir -p "$ROOT/_OSX_RELEASE"

# =========================================================
# Configure + build (universal arm64 + x86_64)
# =========================================================

echo ""; echo "=== Configuring (Release, universal, no user-plugin install) ==="
cmake -S "$ROOT" -B "$BUILD_DIR" \
      -G "Unix Makefiles" \
      -DCMAKE_BUILD_TYPE=Release \
      -DREAPER_LIBSNDFILEWRAPPER_INSTALL_USER_PLUGINS=OFF \
      -DCMAKE_LIBRARY_OUTPUT_DIRECTORY="$BUILD_DIR/out"

echo ""; echo "=== Building ==="
cmake --build "$BUILD_DIR" -j "$(sysctl -n hw.logicalcpu)"

PLUGIN_BUILT="$BUILD_DIR/out/$PLUGIN_NAME"
if [ ! -f "$PLUGIN_BUILT" ]; then
    echo "Error: build did not produce $PLUGIN_BUILT"
    exit 1
fi

# =========================================================
# Stage payload
# =========================================================

PLUGIN_STAGED="$STAGE_DIR$INSTALL_PARENT/$PLUGIN_NAME"
cp -X "$PLUGIN_BUILT" "$PLUGIN_STAGED"
chmod u+w "$PLUGIN_STAGED"
xattr -c "$PLUGIN_STAGED" 2>/dev/null || true

# LGPL: ship the plugin's and libsndfile's license texts alongside the binary.
LICENSE_DIR="$STAGE_DIR$INSTALL_PARENT/$LICENSE_SUBDIR"
mkdir -p "$LICENSE_DIR"
cp "$ROOT/COPYING"            "$LICENSE_DIR/COPYING"
cp "$ROOT/libsndfile/COPYING" "$LICENSE_DIR/libsndfile-COPYING"

echo ""; echo "=== Plugin's resolved dep list (should be system libs only) ==="
otool -L "$PLUGIN_STAGED" | sed 's/^/  /'

# =========================================================
# Codesign
# =========================================================

if ! $SKIP_SIGN; then
    echo ""; echo "=== Codesigning plugin ==="
    codesign_one "$PLUGIN_STAGED"
    codesign --verify --strict --verbose=2 "$PLUGIN_STAGED"
fi

# =========================================================
# pkgbuild + productsign + notarize
# =========================================================

UNSIGNED_PKG="$BUILD_DIR/reaper_libsndfilewrapper_v${VERSION}_macos_unsigned.pkg"
INSTALLER="$ROOT/_OSX_RELEASE/reaper_libsndfilewrapper_v${VERSION}_macos.pkg"

echo ""; echo "=== Building installer payload ==="
# Strip xattrs / AppleDouble shadow files so pkgbuild's payload is clean.
xattr -cr "$STAGE_DIR"
find "$STAGE_DIR" -name "._*" -delete

CLEAN_STAGE="$BUILD_DIR/clean_stage"
rm -rf "$CLEAN_STAGE"
mkdir -p "$CLEAN_STAGE"
ditto --norsrc --noextattr --noacl --noqtn "$STAGE_DIR" "$CLEAN_STAGE"
find "$CLEAN_STAGE" -name "._*" -delete
xattr -cr "$CLEAN_STAGE"

COPYFILE_DISABLE=1 \
COPY_EXTENDED_ATTRIBUTES_DISABLE=1 \
pkgbuild --root "$CLEAN_STAGE" \
         --identifier "com.kronlachner.reaper_libsndfilewrapper" \
         --version "$VERSION" \
         --install-location "/" \
         --ownership recommended \
         "$UNSIGNED_PKG"

if $SKIP_SIGN; then
    mv "$UNSIGNED_PKG" "$INSTALLER"
    echo "Note: package is NOT signed (--no-sign)"
else
    productsign --sign "$CODESIGN_INSTALLER" "$UNSIGNED_PKG" "$INSTALLER"
    rm -f "$UNSIGNED_PKG"
    pkgutil --check-signature "$INSTALLER"

    echo ""; echo "=== Notarizing $(basename "$INSTALLER") ==="
    xcrun notarytool submit "$INSTALLER" \
          --apple-id "$NOTARIZE_APPLE_ID" \
          --password "$NOTARIZE_PASSWORD" \
          --team-id  "$NOTARIZE_TEAM_ID" \
          --wait

    xcrun stapler staple "$INSTALLER"

    echo ""; echo "=== Verifying installer ==="
    stapler validate "$INSTALLER"
    spctl -a -vvv --assess --type install "$INSTALLER"
fi

echo ""; echo "Done!"
ls -la "$INSTALLER"
