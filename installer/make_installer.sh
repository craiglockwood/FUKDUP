#!/bin/bash
set -e

VERSION=$(sed -n 's/^project(FUKDUP VERSION \([0-9.]*\)).*/\1/p' CMakeLists.txt)
VERSION=${VERSION:-1.0.0}
ART=build/FUKDUP_artefacts/Release
ROOT=pkgroot

rm -rf "$ROOT" scripts resources
mkdir -p "$ROOT/Library/Audio/Plug-Ins/Components" "$ROOT/Library/Audio/Plug-Ins/VST3"
cp -R "$ART/AU/FUKDUP.component" "$ROOT/Library/Audio/Plug-Ins/Components/"
cp -R "$ART/VST3/FUKDUP.vst3" "$ROOT/Library/Audio/Plug-Ins/VST3/"

mkdir -p scripts resources

cat > scripts/postinstall << 'EOF'
#!/bin/bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
exit 0
EOF
chmod +x scripts/postinstall

cat > resources/welcome.html << EOF
<html><body style="font-family:-apple-system,Helvetica">
<h2>FUKDUP $VERSION</h2>
<p>An audio destruction unit made by Craig Lockwood.</p>
<p>This installs the FUKDUP Audio Unit and VST3 plugins. Restart your DAW afterwards.</p>
</body></html>
EOF

pkgbuild --analyze --root "$ROOT" components.plist
i=0
while /usr/libexec/PlistBuddy -c "Print :$i" components.plist > /dev/null 2>&1; do
  plutil -replace "$i.BundleIsRelocatable" -bool NO components.plist
  i=$((i+1))
done

pkgbuild --root "$ROOT" --component-plist components.plist --scripts scripts \
  --identifier com.fukdup.fukdup.pkg --version "$VERSION" \
  --install-location / FUKDUP-core.pkg

productbuild --synthesize --package FUKDUP-core.pkg distribution.xml
sed -i '' "s#<installer-gui-script minSpecVersion=\"[0-9]*\">#&<title>FUKDUP $VERSION</title><welcome file=\"welcome.html\" mime-type=\"text/html\"/>#" distribution.xml

productbuild --distribution distribution.xml --resources resources \
  --package-path . "FUKDUP-$VERSION-Installer.pkg"

echo "Built FUKDUP-$VERSION-Installer.pkg"
