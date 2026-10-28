#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
source scripts/env.sh
: "${RESTAURIERUNGSHEFT_QT_ROOT:?Qt-6.8.3-SDK angeben}"
restoration_arch="$(uname -m)"
restoration_version="$(sed -n 's/project(RestaurierungsHeft VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
restoration_name="restaurierungsheft-$restoration_version-macos-$restoration_arch"
restoration_stage="$PWD/build/releases/$restoration_name"
restoration_dmg="$PWD/build/releases/$restoration_name.dmg"
if [[ -e "$restoration_stage" || -e "$restoration_dmg" ]]; then
  echo "Lieferung existiert bereits: $restoration_name" >&2
  exit 1
fi
cmake --preset release -DBUILD_TESTING=ON -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 "-DCMAKE_OSX_ARCHITECTURES=$restoration_arch"
cmake --build --preset release --parallel 4
ctest --preset release
mkdir -p "$restoration_stage"
cp README.md THIRD_PARTY_NOTICES.md "$restoration_stage/"
cp -R docs "$restoration_stage/docs"
cp -R build/release/restaurierungsheft.app "$restoration_stage/RestaurierungsHeft.app"
restoration_app="$restoration_stage/RestaurierungsHeft.app"
"$RESTAURIERUNGSHEFT_QT_ROOT/bin/macdeployqt" "$restoration_app" -always-overwrite -appstore-compliant
test -f "$restoration_app/Contents/PlugIns/sqldrivers/libqsqlite.dylib"
test -f "$restoration_app/Contents/PlugIns/platforms/libqcocoa.dylib"
test -f "$restoration_app/Contents/PlugIns/imageformats/libqjpeg.dylib"
mkdir -p "$restoration_app/Contents/Resources/translations"
cp "$RESTAURIERUNGSHEFT_QT_ROOT/translations/qtbase_de.qm" "$restoration_app/Contents/Resources/translations/"
cat > "$restoration_app/Contents/Resources/qt.conf" <<'QTCONF'
[Paths]
Plugins=PlugIns
Translations=Resources/translations
QTCONF
cp -R licenses "$restoration_app/Contents/Resources/licenses"
cp -R "$RESTAURIERUNGSHEFT_QT_ROOT/sbom" "$restoration_app/Contents/Resources/licenses/qt-sdk-sbom"
cp THIRD_PARTY_NOTICES.md "$restoration_app/Contents/Resources/"
restoration_icons="$PWD/build/RestaurierungsHeft.iconset"
mkdir -p "$restoration_icons"
for restoration_size in 16 32 128 256 512; do
  sips -z "$restoration_size" "$restoration_size" assets/branding/icon-1024.png --out "$restoration_icons/icon_${restoration_size}x${restoration_size}.png" >/dev/null
  restoration_double=$((restoration_size * 2))
  sips -z "$restoration_double" "$restoration_double" assets/branding/icon-1024.png --out "$restoration_icons/icon_${restoration_size}x${restoration_size}@2x.png" >/dev/null
done
iconutil -c icns "$restoration_icons" -o "$restoration_app/Contents/Resources/RestaurierungsHeft.icns"
/usr/libexec/PlistBuddy -c "Add :CFBundleIconFile string RestaurierungsHeft.icns" "$restoration_app/Contents/Info.plist"
# Ad-hoc signing permits local Apple-Silicon testing. It is not a distribution identity.
codesign --force --deep --sign - "$restoration_app"
hdiutil create -volname RestaurierungsHeft -srcfolder "$restoration_stage" -ov -format UDZO "$restoration_dmg"
shasum -a 256 "$restoration_dmg" > "$restoration_dmg.sha256"
echo "$restoration_dmg"
echo "Native GUI-Abnahme und spätere Store-Signierung bleiben erforderlich."
