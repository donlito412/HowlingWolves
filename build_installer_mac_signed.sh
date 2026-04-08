#!/bin/bash
set -e

# ============================================================
# Wolf Instruments - macOS Sign, Package, Notarize & Staple
# Run from the project root (same folder as CMakeLists.txt)
# ============================================================
# Set these environment variables before running locally:
#
#   export APPLE_ID="djdonlito412@gmail.com"
#   export APPLE_APP_SPECIFIC_PASSWORD="your-app-specific-password"
#   export APPLE_TEAM_ID="9PCW5L2HBB"
#
# In GitHub Actions these are injected automatically via secrets.
# ============================================================

PLUGIN_NAME="Howling Wolves"
PLUGIN_VERSION="1.0.20"
IDENTIFIER="com.donlito412.howlingwolves"

BUILD_ROOT="build/HowlingWolves_artefacts/Release"
STAGING_DIR="Installer/staging_signed"
OUTPUT_DIR="Installer/output_signed"

DEV_ID_APP="Developer ID Application: Jonathan Freeman (9PCW5L2HBB)"
DEV_ID_INSTALLER="Developer ID Installer: Jonathan Freeman (9PCW5L2HBB)"

VST3="$BUILD_ROOT/VST3/$PLUGIN_NAME.vst3"
AU="$BUILD_ROOT/AU/$PLUGIN_NAME.component"
APP="$BUILD_ROOT/Standalone/$PLUGIN_NAME.app"

if [ -z "$APPLE_ID" ] || [ -z "$APPLE_APP_SPECIFIC_PASSWORD" ] || [ -z "$APPLE_TEAM_ID" ]; then
    echo "ERROR: Missing required environment variables."
    echo "  export APPLE_ID=..."
    echo "  export APPLE_APP_SPECIFIC_PASSWORD=(...)"
    echo "  export APPLE_TEAM_ID=..."
    exit 1
fi

for BINARY in "$VST3" "$AU" "$APP"; do
    if [ ! -e "$BINARY" ]; then
        echo "ERROR: Binary not found: $BINARY"
        exit 1
    fi
done

echo "=== Cleaning staging directories ==="
rm -rf "$STAGING_DIR" "$OUTPUT_DIR"
mkdir -p "$STAGING_DIR/vst3_payload/Library/Audio/Plug-Ins/VST3"
mkdir -p "$STAGING_DIR/au_payload/Library/Audio/Plug-Ins/Components"
mkdir -p "$STAGING_DIR/content_payload/Users/Shared/Wolf Instruments"
mkdir -p "$STAGING_DIR/scripts"
mkdir -p "$OUTPUT_DIR"

echo "=== Signing VST3 ==="
codesign --force --verify --verbose --sign "$DEV_ID_APP" --options runtime --timestamp "$VST3"

echo "=== Signing AU ==="
codesign --force --verify --verbose --sign "$DEV_ID_APP" --options runtime --timestamp "$AU"

echo "=== Signing Standalone App ==="
if [ -d "$APP/Contents/Frameworks" ]; then
    find "$APP/Contents/Frameworks" -name "*.dylib" | while read LIB; do
        codesign --force --sign "$DEV_ID_APP" --options runtime --timestamp "$LIB"
    done
    find "$APP/Contents/Frameworks" -name "*.framework" | while read FW; do
        codesign --force --sign "$DEV_ID_APP" --options runtime --timestamp "$FW"
    done
fi
codesign --force --verify --verbose --sign "$DEV_ID_APP" --options runtime --timestamp "$APP"

echo "=== Verifying signatures ==="
codesign --verify --deep --strict --verbose=2 "$VST3"
codesign --verify --deep --strict --verbose=2 "$AU"
codesign --verify --deep --strict --verbose=2 "$APP"

echo "=== Staging signed files ==="
cp -R "$VST3" "$STAGING_DIR/vst3_payload/Library/Audio/Plug-Ins/VST3/"
cp -R "$AU" "$STAGING_DIR/au_payload/Library/Audio/Plug-Ins/Components/"

if [ -d "DistributionContent" ]; then
    cp -R "DistributionContent/"* "$STAGING_DIR/content_payload/Users/Shared/Wolf Instruments/"
fi

cat <<'EOT' > "$STAGING_DIR/scripts/postinstall"
#!/bin/bash
if [ -d "/Users/Shared/Wolf Instruments" ]; then
    chmod -R 777 "/Users/Shared/Wolf Instruments" || true
fi
exit 0
EOT
chmod +x  "$STAGING_DIR/scripts/postinstall"

echo "=== Building component packages ==="
pkgbuild --root "$STAGING_DIR/vst3_payload" --identifier "$IDENTIFIER.vst3" --version "$PLUGIN_VERSION" --install-location "/" "$STAGING_DIR/vst3.pkg"
pkgbuild --root "$STAGING_DIR/au_payload" --identifier "$IDENTIFIER.au" --version "$PLUGIN_VERSION" --install-location "/" "$STAGING_DIR/au.pkg"
pkgbuild --root "$STAGING_DIR/content_payload" --identifier "$IDENTIFIER.content" --version "$PLUGIN_VERSION" --install-location "/" --scripts "$STAGING_DIR/scripts" "$STAGING_DIR/content.pkg"

echo "=== Synthesizing distribution and building installer ==="
productbuild --synthesize --package "$STAGING_DIR/vst3.pkg" --package "$STAGING_DIR/au.pkg" --package "$STAGING_DIR/content.pkg" "$STAGING_DIR/distribution.xml"

# Remove OS upper bound from synthesized distribution.xml (macOS 15+ / Sequoia).
# Replace whole <allowed-os-versions>…</> so we do not re-serialize XML (ElementTree can break productbuild).
python3 - "$STAGING_DIR/distribution.xml" <<'PATCH_DIST_PY'
import re
import sys

path = sys.argv[1]
with open(path, "r", encoding="utf-8") as f:
    content = f.read()

replacement = "<allowed-os-versions><os-version min=\"10.13\"/></allowed-os-versions>"
new_content, n = re.subn(
    r"<allowed-os-versions[^>]*>.*?</allowed-os-versions>",
    replacement,
    content,
    flags=re.DOTALL,
)
if n == 0:
    # No block (unusual): strip Apple upper-bound attrs without rewriting whole file
    new_content = re.sub(r'\s+before="[^"]*"', "", content)
    new_content = re.sub(r'\s+max="[^"]*"', "", new_content)

with open(path, "w", encoding="utf-8") as f:
    f.write(new_content)
PATCH_DIST_PY
echo "=== Patched distribution.xml (allowed-os-versions: min 10.13, no upper cap) ==="

UNSIGNED_PKG="$OUTPUT_DIR/${PLUGIN_NAME}_unsigned.pkg"
SIGNED_PKG="$OUTPUT_DIR/${PLUGIN_NAME} Installer.pkg"

productbuild --distribution "$STAGING_DIR/distribution.xml" --package-path "$STAGING_DIR" "$UNSIGNED_PKG"

echo "=== Signing installer ==="
productsign --sign "$DEV_ID_INSTALLER" --timestamp "$UNSIGNED_PKG" "$SIGNED_PKG"
rm -f "$UNSIGNED_PKG"

pkgutil --check-signature "$SIGNED_PKG"

echo "=== Submitting for notarization ==="
xcrun notarytool submit "$SIGNED_PKG" --apple-id "$APPLE_ID" --password "$APPLE_APP_SPECIFIC_PASSWORD" --team-id "$APPLE_TEAM_ID" --wait

xcrun stapler staple "$SIGNED_PKG"
xcrun stapler validate "$SIGNED_PKG"

echo ""
echo "COMPLETE: $SIGNED_PKG"
echo "Signed, Notarized, Stapled - Ready for distribution"
