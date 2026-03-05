#!/bin/bash
PLUGIN_NAME="Howling Wolves"
PLUGIN_VERSION="1.0.19"
IDENTIFIER="com.donlito412.howlingwolves"
BUILD_ROOT="build/HowlingWolves_artefacts"
STAGING_DIR="Installer/staging_mac"
OUTPUT_DIR="."

rm -rf "$STAGING_DIR"
mkdir -p "$STAGING_DIR/vst3_payload/Library/Audio/Plug-Ins/VST3"
mkdir -p "$STAGING_DIR/au_payload/Library/Audio/Plug-Ins/Components"
mkdir -p "$STAGING_DIR/content_payload/Users/Shared/Wolf Instruments"
mkdir -p "$STAGING_DIR/scripts"

if [ -d "$BUILD_ROOT/Release/VST3" ]; then
    cp -R "$BUILD_ROOT/Release/VST3/"* "$STAGING_DIR/vst3_payload/Library/Audio/Plug-Ins/VST3/" 2>/dev/null || true
    cp -R "$BUILD_ROOT/Release/AU/"* "$STAGING_DIR/au_payload/Library/Audio/Plug-Ins/Components/" 2>/dev/null || true
elif [ -d "$BUILD_ROOT/VST3" ]; then
    cp -R "$BUILD_ROOT/VST3/"* "$STAGING_DIR/vst3_payload/Library/Audio/Plug-Ins/VST3/" 2>/dev/null || true
    cp -R "$BUILD_ROOT/AU/"* "$STAGING_DIR/au_payload/Library/Audio/Plug-Ins/Components/" 2>/dev/null || true
fi

cat <<'EOT' > "$STAGING_DIR/scripts/postinstall"
#!/bin/bash
TARGET_DIR="/Users/Shared/Wolf Instruments"
if [ -d "$TARGET_DIR" ]; then
    chmod -R 777 "$TARGET_DIR" || true
fi
exit 0
EOT
chmod +x "$STAGING_DIR/scripts/postinstall"

pkgbuild --root "$STAGING_DIR/vst3_payload" --identifier "$IDENTIFIER.vst3" --version "$PLUGIN_VERSION" --install-location "/" "$STAGING_DIR/vst3.pkg"
pkgbuild --root "$STAGING_DIR/au_payload" --identifier "$IDENTIFIER.au" --version "$PLUGIN_VERSION" --install-location "/" "$STAGING_DIR/au.pkg"
pkgbuild --root "$STAGING_DIR/content_payload" --identifier "$IDENTIFIER.content" --version "$PLUGIN_VERSION" --install-location "/" --scripts "$STAGING_DIR/scripts" "$STAGING_DIR/content.pkg"

productbuild --synthesize --package "$STAGING_DIR/vst3.pkg" --package "$STAGING_DIR/au.pkg" --package "$STAGING_DIR/content.pkg" "$STAGING_DIR/distribution.xml"
productbuild --distribution "$STAGING_DIR/distribution.xml" --package-path "$STAGING_DIR" "$OUTPUT_DIR/HowlingWolves_Mac_Installer.pkg"
