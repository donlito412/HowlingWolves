#!/bin/bash
set -ex

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

ls -R "$BUILD_ROOT" || true

if [ -d "$BUILD_ROOT/Release/VST3" ]; then
    echo "Found Release/VST3"
    cp -R "$BUILD_ROOT/Release/VST3/"* "$STAGING_DIR/vst3_payload/Library/Audio/Plug-Ins/VST3/" || true
    cp -R "$BUILD_ROOT/Release/AU/"* "$STAGING_DIR/au_payload/Library/Audio/Plug-Ins/Components/" || true
elif [ -d "$BUILD_ROOT/VST3" ]; then
    echo "Found VST3"
    cp -R "$BUILD_ROOT/VST3/"* "$STAGING_DIR/vst3_payload/Library/Audio/Plug-Ins/VST3/" || true
    cp -R "$BUILD_ROOT/AU/"* "$STAGING_DIR/au_payload/Library/Audio/Plug-Ins/Components/" || true
else
    echo "ERROR: COULD NOT FIND BUILT VST3 DIRECTORIES!"
    exit 1
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

# Same as build_installer_mac_signed.sh: allow macOS 15+ by removing synthesized OS upper bound.
python3 - "$STAGING_DIR/distribution.xml" <<'PATCH_DIST_PY'
import sys
import xml.etree.ElementTree as ET

path = sys.argv[1]
tree = ET.parse(path)
root = tree.getroot()

def local_name(tag):
    return tag.split("}", 1)[-1] if tag.startswith("{") else tag

for elem in root.iter():
    if local_name(elem.tag) != "os-version":
        continue
    elem.set("min", "10.13")
    for attr in list(elem.attrib):
        ln = local_name(attr) if "}" in attr else attr
        if ln in ("before", "max"):
            del elem.attrib[attr]

tree.write(path, encoding="utf-8", xml_declaration=True)
PATCH_DIST_PY

productbuild --distribution "$STAGING_DIR/distribution.xml" --package-path "$STAGING_DIR" "$OUTPUT_DIR/HowlingWolves_Mac_Installer.pkg"

ls -la "$OUTPUT_DIR"
