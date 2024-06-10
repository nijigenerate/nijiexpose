# First build ARM64 version...
echo "Building arm64 binary..."
dub build --build=release --config=osx-full --arch=arm64-apple-macos
mv "out/nijiexpose.app/Contents/MacOS/nijiexpose" "out/nijiexpose.app/Contents/MacOS/nijiexpose-arm64"

# Then the X86_64 version...
echo "Building x86_64 binary..."
dub build --build=release --config=osx-full --arch=x86_64-apple-macos
mv "out/nijiexpose.app/Contents/MacOS/nijiexpose" "out/nijiexpose.app/Contents/MacOS/nijiexpose-x86_64"

# Glue them together with lipo
echo "Gluing them together..."
lipo "out/nijiexpose.app/Contents/MacOS/nijiexpose-x86_64" "out/nijiexpose.app/Contents/MacOS/nijiexpose-arm64" -output "out/nijiexpose.app/Contents/MacOS/nijiexpose" -create

# Print some nice info
echo "Done!"
lipo -info "out/nijiexpose.app/Contents/MacOS/nijiexpose"

# Cleanup and bundle
echo "Cleaning up..."
rm "out/nijiexpose.app/Contents/MacOS/nijiexpose-x86_64" "out/nijiexpose.app/Contents/MacOS/nijiexpose-arm64"
./osxbundle.sh