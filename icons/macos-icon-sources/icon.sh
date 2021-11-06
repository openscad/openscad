mkdir OpenSCAD.iconset
sips -z 16 16     Icon1024.png --out OpenSCAD.iconset/icon_16x16.png
sips -z 32 32     Icon1024.png --out OpenSCAD.iconset/icon_16x16@2x.png
sips -z 32 32     Icon1024.png --out OpenSCAD.iconset/icon_32x32.png
sips -z 64 64     Icon1024.png --out OpenSCAD.iconset/icon_32x32@2x.png
sips -z 128 128   Icon1024.png --out OpenSCAD.iconset/icon_128x128.png
sips -z 256 256   Icon1024.png --out OpenSCAD.iconset/icon_128x128@2x.png
sips -z 256 256   Icon1024.png --out OpenSCAD.iconset/icon_256x256.png
sips -z 512 512   Icon1024.png --out OpenSCAD.iconset/icon_256x256@2x.png
sips -z 512 512   Icon1024.png --out OpenSCAD.iconset/icon_512x512.png
cp Icon1024.png OpenSCAD.iconset/icon_512x512@2x.png
iconutil -c icns OpenSCAD.iconset
rm -R OpenSCAD.iconset

mkdir icon-nightly.iconset
sips -z 16 16     Icon1024Nightly.png --out icon-nightly.iconset/icon_16x16.png
sips -z 32 32     Icon1024Nightly.png --out icon-nightly.iconset/icon_16x16@2x.png
sips -z 32 32     Icon1024Nightly.png --out icon-nightly.iconset/icon_32x32.png
sips -z 64 64     Icon1024Nightly.png --out icon-nightly.iconset/icon_32x32@2x.png
sips -z 128 128   Icon1024Nightly.png --out icon-nightly.iconset/icon_128x128.png
sips -z 256 256   Icon1024Nightly.png --out icon-nightly.iconset/icon_128x128@2x.png
sips -z 256 256   Icon1024Nightly.png --out icon-nightly.iconset/icon_256x256.png
sips -z 512 512   Icon1024Nightly.png --out icon-nightly.iconset/icon_256x256@2x.png
sips -z 512 512   Icon1024Nightly.png --out icon-nightly.iconset/icon_512x512.png
cp Icon1024Nightly.png icon-nightly.iconset/icon_512x512@2x.png
iconutil -c icns icon-nightly.iconset
rm -R icon-nightly.iconset
