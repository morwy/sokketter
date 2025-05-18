#!/bin/bash

mkdir socket-icon.iconset

sips -z 16 16 socket-icon-1024x1024.png --out socket-icon.iconset/icon_16x16.png
sips -z 32 32 socket-icon-1024x1024.png --out socket-icon.iconset/icon_16x16@2x.png
sips -z 32 32 socket-icon-1024x1024.png --out socket-icon.iconset/icon_32x32.png
sips -z 64 64 socket-icon-1024x1024.png --out socket-icon.iconset/icon_32x32@2x.png
sips -z 128 128 socket-icon-1024x1024.png --out socket-icon.iconset/icon_128x128.png
sips -z 256 256 socket-icon-1024x1024.png --out socket-icon.iconset/icon_128x128@2x.png
sips -z 256 256 socket-icon-1024x1024.png --out socket-icon.iconset/icon_256x256.png
sips -z 512 512 socket-icon-1024x1024.png --out socket-icon.iconset/icon_256x256@2x.png
sips -z 512 512 socket-icon-1024x1024.png --out socket-icon.iconset/icon_512x512.png
cp socket-icon-1024x1024.png socket-icon.iconset/icon_512x512@2x.png

iconutil -c icns socket-icon.iconset

rm -R socket-icon.iconset
