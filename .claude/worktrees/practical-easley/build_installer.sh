#!/bin/bash

# Configuration
PLUGIN_NAME="Howling Wolves"
PLUGIN_VERSION="1.0.18"
IDENTIFIER="com.donlito412.howlingwolves"
BUILD_ROOT="build_v18/HowlingWolves_artefacts"
STAGING_DIR="Installer/staging_v18"
OUTPUT_DIR="Installer/output"

# Clean
rm -rf "Installer"
mkdir -p "$STAGING_DIR/vst3"
mkdir -p "$STAGING_DIR/au"
mkdir -p "$STAGING_DIR/content"
mkdir -p "$OUTPUT_DIR"

echo "Staging VST3..."
# Create structure for pkgbuild --root
# pkgbuild installs the CONTENTS of the root folder to the install-location
# But for plugins, we usually want to install to /Library/Audio/Plug-Ins/VST3
# So we structure the root as:
# staging/vst3_payload/Library/Audio/Plug-Ins/VST3/Howling Wolves.vst3

mkdir -p "$STAGING_DIR/vst3_payload/Library/Audio/Plug-Ins/VST3"
cp -R "$BUILD_ROOT/Release/VST3/$PLUGIN_NAME.vst3" "$STAGING_DIR/vst3_payload/Library/Audio/Plug-Ins/VST3/"

echo "Staging AU..."
mkdir -p "$STAGING_DIR/au_payload/Library/Audio/Plug-Ins/Components"
cp -R "$BUILD_ROOT/Release/AU/$PLUGIN_NAME.component" "$STAGING_DIR/au_payload/Library/Audio/Plug-Ins/Components/"

echo "Staging Standalone App..."
mkdir -p "$STAGING_DIR/app_payload/Applications"
cp -R "$BUILD_ROOT/Release/Standalone/$PLUGIN_NAME.app" "$STAGING_DIR/app_payload/Applications/"

echo "Staging Content..."
# Install to /Users/Shared/Wolf Instruments/Factory Presets
# Note: Installer usually runs as root. Files will be owned by root.
# We might need a post-install script to chmod/chown them so users can read them?
# /Users/Shared is world readable/writable usually (sticky bit).
# Better path: /Users/Shared/Wolf Instruments
mkdir -p "$STAGING_DIR/content_payload/Users/Shared/Wolf Instruments"
# Assuming content exists in a local "Wolf Instruments Content" folder or similar
# For now, we'll assume the user has a "DistributionContent" folder to copy from
# IF NOT EXIST, we create dummy for testing or warn
if [ -d "DistributionContent" ]; then
    cp -R "DistributionContent/"* "$STAGING_DIR/content_payload/Users/Shared/Wolf Instruments/"
else
    echo "WARNING: DistributionContent folder not found. Creating placeholder."
    mkdir -p "$STAGING_DIR/content_payload/Users/Shared/Wolf Instruments/Factory Presets"
    touch "$STAGING_DIR/content_payload/Users/Shared/Wolf Instruments/Factory Presets/README.txt"
fi

# Create scripts directory
mkdir -p "$STAGING_DIR/scripts"

# Create postinstall script to fix permissions
cat <<EOT > "$STAGING_DIR/scripts/postinstall"
#!/bin/bash
# Fix permissions for the Shared content so all users can read/write
TARGET_DIR="/Users/Shared/Wolf Instruments"
if [ -d "\$TARGET_DIR" ]; then
    /bin/chmod -R 777 "\$TARGET_DIR" || true
fi
exit 0
EOT

chmod +x "$STAGING_DIR/scripts/postinstall"

echo "Building Component Packages..."

# VST3 Package
pkgbuild --root "$STAGING_DIR/vst3_payload" \
         --identifier "$IDENTIFIER.vst3" \
         --version "$PLUGIN_VERSION" \
         --install-location "/" \
         "$STAGING_DIR/vst3.pkg"

# AU Package
pkgbuild --root "$STAGING_DIR/au_payload" \
         --identifier "$IDENTIFIER.au" \
         --version "$PLUGIN_VERSION" \
         --install-location "/" \
         "$STAGING_DIR/au.pkg"

# App Package
pkgbuild --root "$STAGING_DIR/app_payload" \
         --identifier "$IDENTIFIER.app" \
         --version "$PLUGIN_VERSION" \
         --install-location "/" \
         "$STAGING_DIR/app.pkg"

# Content Package
pkgbuild --root "$STAGING_DIR/content_payload" \
         --identifier "$IDENTIFIER.content" \
         --version "$PLUGIN_VERSION" \
         --install-location "/" \
         --scripts "$STAGING_DIR/scripts" \
         "$STAGING_DIR/content.pkg"

echo "Synthesizing Distribution XML..."
productbuild --synthesize \
             --package "$STAGING_DIR/vst3.pkg" \
             --package "$STAGING_DIR/au.pkg" \
             --package "$STAGING_DIR/app.pkg" \
             --package "$STAGING_DIR/content.pkg" \
             "$STAGING_DIR/distribution.xml"

# (Logic to manually edit distribution.xml would go here to set titles/descriptions)
# For now, we use the synthesized one.

echo "Building Final Installer..."
productbuild --distribution "$STAGING_DIR/distribution.xml" \
             --package-path "$STAGING_DIR" \
             --resources "Installer/resources" \
             "$OUTPUT_DIR/$PLUGIN_NAME Installer.pkg"

echo "Done. Installer is in $OUTPUT_DIR"
