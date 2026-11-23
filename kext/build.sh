#!/bin/bash
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
OUT="$HERE/GiMATEEC.kext"
KHDR="$(xcrun --show-sdk-path)/System/Library/Frameworks/Kernel.framework/Headers"
mkdir -p "$OUT/Contents/MacOS"
xcrun clang++ -arch x86_64 -fapple-kext -mkernel -nostdinc -I"$KHDR" \
  -DKERNEL -DKERNEL_PRIVATE -DDRIVER_PRIVATE -DAPPLE -DNeXT \
  -std=c++17 -fno-rtti -fno-exceptions -fno-builtin -fno-common \
  -Wno-unused-parameter -Wno-inconsistent-missing-override -O2 \
  -c "$HERE/src/GiMATEEC.cpp" -o "$HERE/GiMATEEC.o"
xcrun clang++ -arch x86_64 -fapple-kext -nostdlib -Xlinker -kext \
  "$HERE/GiMATEEC.o" -o "$OUT/Contents/MacOS/GiMATEEC" 2>&1 | head -20
cp "$HERE/src/Info.plist" "$OUT/Contents/Info.plist"
rm -rf "$OUT/Contents/_CodeSignature"
codesign -f -s - "$OUT"
codesign --verify "$OUT"
echo "OK $(shasum -a 256 "$OUT/Contents/MacOS/GiMATEEC" | cut -c1-16)"
