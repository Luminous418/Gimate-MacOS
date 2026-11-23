#!/bin/bash
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
APP="$HERE/GiMATE.app"
rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS"
cp "$HERE/Info.plist" "$APP/Contents/Info.plist"
xcrun swiftc -O -parse-as-library \
  -target x86_64-apple-macos26.0 \
  -o "$APP/Contents/MacOS/GiMATE" \
  "$HERE"/src/*.swift \
  -framework AppKit -framework SwiftUI -framework IOKit -framework CoreFoundation 2>&1 | head -30
codesign -f -s - "$APP"
codesign --verify "$APP"
echo "OK $APP"
