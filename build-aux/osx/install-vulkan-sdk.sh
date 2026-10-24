#!/usr/bin/env bash
set -euo pipefail

# Install only on a CI SDK-cache miss. The SDK remains outside the build cache
# used by MediaPipe. Optional system-wide and iOS components are not installed.
version="${VULKAN_SDK_VERSION:?}"
checksum="${VULKAN_SDK_SHA256:?}"
sdk_root="$PWD/build/vulkan-sdk/$version"
download_dir="${RUNNER_TEMP:?}/nijiexpose-vulkan-$version"
archive="$download_dir/vulkansdk-macos-$version.zip"
mkdir -p "$download_dir" "$(dirname "$sdk_root")"
curl --fail --location --retry 3 --connect-timeout 30 --max-time 600 \
    "https://sdk.lunarg.com/sdk/download/$version/mac/vulkansdk-macos-$version.zip" \
    --output "$archive"
printf '%s  %s\n' "$checksum" "$archive" | shasum -a 256 --check
ditto -x -k "$archive" "$download_dir"
installers=("$download_dir"/vulkansdk-*.app/Contents/MacOS/vulkansdk-*)
if [[ ${#installers[@]} != 1 || ! -x "${installers[0]}" ]]; then
    echo "Expected one Vulkan SDK installer in $download_dir" >&2
    exit 1
fi
"${installers[0]}" --root "$sdk_root" --accept-licenses \
    --default-answer --confirm-command copy_only=1 install com.lunarg.vulkan.core
