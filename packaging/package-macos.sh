#!/usr/bin/env bash
#
# Builds the macOS installer (.pkg) for Gesture Synth.
#
# Works without any Apple credentials (produces an unsigned installer).
# Signing and notarization switch on automatically when available:
#   - a "Developer ID Application" identity in the keychain -> bundles are codesigned
#   - a "Developer ID Installer" identity in the keychain   -> the pkg is signed
#   - NOTARIZATION_APPLE_ID + NOTARIZATION_PASSWORD + APPLE_TEAM_ID set
#                                                           -> the pkg is notarized and stapled
#
# Required env:
#   ARTIFACTS_PATH  directory containing Standalone/ VST3/ AU/ CLAP/ bundle dirs
#   PRODUCT_NAME    e.g. "Gesture Synth"
#   VERSION         e.g. 0.1.0
#   BUNDLE_ID       e.g. com.tylerteuber.gesturesynth
# Optional env:
#   OUTPUT_DIR      where the final pkg lands       (default: ARTIFACTS_PATH)
#   ARTIFACT_NAME   final pkg name without .pkg     (default: "$PRODUCT_NAME-$VERSION-macOS")

set -euo pipefail

: "${ARTIFACTS_PATH:?ARTIFACTS_PATH must point at the built artefacts directory}"
: "${PRODUCT_NAME:?PRODUCT_NAME is required}"
: "${VERSION:?VERSION is required}"
: "${BUNDLE_ID:?BUNDLE_ID is required}"
OUTPUT_DIR="${OUTPUT_DIR:-$ARTIFACTS_PATH}"
ARTIFACT_NAME="${ARTIFACT_NAME:-$PRODUCT_NAME-$VERSION-macOS}"

PACKAGING_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

APP_BUNDLE="$ARTIFACTS_PATH/Standalone/$PRODUCT_NAME.app"
VST3_BUNDLE="$ARTIFACTS_PATH/VST3/$PRODUCT_NAME.vst3"
AU_BUNDLE="$ARTIFACTS_PATH/AU/$PRODUCT_NAME.component"
CLAP_BUNDLE="$ARTIFACTS_PATH/CLAP/$PRODUCT_NAME.clap"

for bundle in "$APP_BUNDLE" "$VST3_BUNDLE" "$AU_BUNDLE" "$CLAP_BUNDLE"; do
    [[ -d "$bundle" ]] || { echo "error: missing bundle: $bundle" >&2; exit 1; }
done

# ---------------------------------------------------------------------------
# Detect signing identities (absent -> unsigned build)
# ---------------------------------------------------------------------------
DEV_ID_APP="$(security find-identity -v -p codesigning 2>/dev/null \
    | awk -F'"' '/Developer ID Application/ { print $2; exit }' || true)"
DEV_ID_INSTALLER="$(security find-identity -v 2>/dev/null \
    | awk -F'"' '/Developer ID Installer/ { print $2; exit }' || true)"

if [[ -n "$DEV_ID_APP" ]]; then
    echo "Codesigning with: $DEV_ID_APP"

    sign() {
        codesign --force --options runtime --timestamp --sign "$DEV_ID_APP" "$1"
    }

    # Inner bundles (embedded AUv3 appex) must be signed before their container
    for appex in "$APP_BUNDLE/Contents/PlugIns/"*.appex; do
        [[ -e "$appex" ]] && sign "$appex"
    done
    sign "$APP_BUNDLE"
    sign "$VST3_BUNDLE"
    sign "$AU_BUNDLE"
    sign "$CLAP_BUNDLE"
else
    echo "No 'Developer ID Application' identity found — bundles stay unsigned"
fi

# ---------------------------------------------------------------------------
# Component packages
# ---------------------------------------------------------------------------
build_component_pkg() {
    local bundle="$1" kind="$2" install_location="$3"
    pkgbuild \
        --component "$bundle" \
        --identifier "$BUNDLE_ID.$kind.pkg" \
        --version "$VERSION" \
        --install-location "$install_location" \
        "$WORK_DIR/$PRODUCT_NAME.$kind.pkg"
}

build_component_pkg "$APP_BUNDLE"  app  "/Applications"
build_component_pkg "$VST3_BUNDLE" vst3 "/Library/Audio/Plug-Ins/VST3"
build_component_pkg "$AU_BUNDLE"   au   "/Library/Audio/Plug-Ins/Components"
build_component_pkg "$CLAP_BUNDLE" clap "/Library/Audio/Plug-Ins/CLAP"

# ---------------------------------------------------------------------------
# Distribution package
# ---------------------------------------------------------------------------
sed -e "s/\${PRODUCT_NAME}/$PRODUCT_NAME/g" \
    -e "s/\${BUNDLE_ID}/$BUNDLE_ID/g" \
    -e "s/\${VERSION}/$VERSION/g" \
    "$PACKAGING_DIR/distribution.xml.template" > "$WORK_DIR/distribution.xml"

mkdir -p "$OUTPUT_DIR"
FINAL_PKG="$OUTPUT_DIR/$ARTIFACT_NAME.pkg"

PRODUCTBUILD_ARGS=(
    --distribution "$WORK_DIR/distribution.xml"
    --package-path "$WORK_DIR"
    --resources "$PACKAGING_DIR/resources"
    --version "$VERSION"
)
if [[ -n "$DEV_ID_INSTALLER" ]]; then
    echo "Signing installer with: $DEV_ID_INSTALLER"
    PRODUCTBUILD_ARGS+=(--sign "$DEV_ID_INSTALLER")
else
    echo "No 'Developer ID Installer' identity found — pkg stays unsigned"
fi

productbuild "${PRODUCTBUILD_ARGS[@]}" "$FINAL_PKG"

# ---------------------------------------------------------------------------
# Notarization (requires a signed pkg)
# ---------------------------------------------------------------------------
if [[ -n "${NOTARIZATION_APPLE_ID:-}" && -n "${NOTARIZATION_PASSWORD:-}" && -n "${APPLE_TEAM_ID:-}" ]]; then
    echo "Submitting for notarization..."
    xcrun notarytool submit "$FINAL_PKG" \
        --apple-id "$NOTARIZATION_APPLE_ID" \
        --password "$NOTARIZATION_PASSWORD" \
        --team-id "$APPLE_TEAM_ID" \
        --wait
    xcrun stapler staple "$FINAL_PKG"
else
    echo "Notarization credentials not set — skipping notarization"
fi

echo "Installer created: $FINAL_PKG"
