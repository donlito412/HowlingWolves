set -e # Exit on error
export DEVELOPER_DIR="/Volumes/easystore/Xcode.app/Contents/Developer"

echo "=== 0. Cleaning Build Directory ==="
rm -rf build_v18 || true

echo "=== 1. Configuring Project (CMake) ==="
# Ensure we are generating for Xcode or Ninja depending on what's available, 
# but usually just `cmake -B build` is enough to set up default generator.
/Volumes/easystore/CMake.app/Contents/bin/cmake -B build_v18 -G "Xcode"

echo "=== 2. Compiling Source Code (Release Mode) ==="
# This is the critical step that was missing!
/Volumes/easystore/CMake.app/Contents/bin/cmake --build build_v18 --config Release --parallel 4

echo "=== 3. Packaging Installer ==="
# Now that binaries are fresh, package them.
./build_installer.sh

echo "=== SUCCESS ==="
echo "New installer created at: Installer/output/Howling Wolves Installer.pkg"
echo "Please run this installer now."
