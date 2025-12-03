#!/bin/bash
set -e

echo "Building for Release..."

# Create build directory
rm -rf build_release
mkdir -p build_release
cd build_release

# Configure with Release type
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
make -j$(nproc)

# Strip binary to reduce size
strip bin/music_player

echo "Build complete."

# Create distribution folder
cd ..
rm -rf dist
mkdir -p dist/music_player
cp build_release/bin/music_player dist/music_player/
cp README.md dist/music_player/ 2>/dev/null || touch dist/music_player/README.txt

# Create a run script
cat <<EOF > dist/music_player/run.sh
#!/bin/bash
cd "\$(dirname "\$0")"
./music_player "\$@"
EOF
chmod +x dist/music_player/run.sh

# Archive
tar -czf music_player_linux_x64.tar.gz -C dist music_player

echo "Package created: music_player_linux_x64.tar.gz"
