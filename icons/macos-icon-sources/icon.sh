mkdir OpenSCAD.iconset
sips -z 16 16     ../openscad-macos.png --out OpenSCAD.iconset/icon_16x16.png
sips -z 32 32     ../openscad-macos.png --out OpenSCAD.iconset/icon_16x16@2x.png
sips -z 32 32     ../openscad-macos.png --out OpenSCAD.iconset/icon_32x32.png
sips -z 64 64     ../openscad-macos.png --out OpenSCAD.iconset/icon_32x32@2x.png
sips -z 128 128   ../openscad-macos.png --out OpenSCAD.iconset/icon_128x128.png
sips -z 256 256   ../openscad-macos.png --out OpenSCAD.iconset/icon_128x128@2x.png
sips -z 256 256   ../openscad-macos.png --out OpenSCAD.iconset/icon_256x256.png
sips -z 512 512   ../openscad-macos.png --out OpenSCAD.iconset/icon_256x256@2x.png
sips -z 512 512   ../openscad-macos.png --out OpenSCAD.iconset/icon_512x512.png
cp ../openscad-macos.png OpenSCAD.iconset/icon_512x512@2x.png
iconutil -c icns OpenSCAD.iconset
mv OpenSCAD.icns ../
rm -R OpenSCAD.iconset

mkdir icon-nightly.iconset
sips -z 16 16     ../openscad-nightly-macos.png --out icon-nightly.iconset/icon_16x16.png
sips -z 32 32     ../openscad-nightly-macos.png --out icon-nightly.iconset/icon_16x16@2x.png
sips -z 32 32     ../openscad-nightly-macos.png --out icon-nightly.iconset/icon_32x32.png
sips -z 64 64     ../openscad-nightly-macos.png --out icon-nightly.iconset/icon_32x32@2x.png
sips -z 128 128   ../openscad-nightly-macos.png --out icon-nightly.iconset/icon_128x128.png
sips -z 256 256   ../openscad-nightly-macos.png --out icon-nightly.iconset/icon_128x128@2x.png
sips -z 256 256   ../openscad-nightly-macos.png --out icon-nightly.iconset/icon_256x256.png
sips -z 512 512   ../openscad-nightly-macos.png --out icon-nightly.iconset/icon_256x256@2x.png
sips -z 512 512   ../openscad-nightly-macos.png --out icon-nightly.iconset/icon_512x512.png
cp ../openscad-nightly-macos.png icon-nightly.iconset/icon_512x512@2x.png
iconutil -c icns icon-nightly.iconset
mv icon-nightly.icns ../
rm -R icon-nightly.iconset
