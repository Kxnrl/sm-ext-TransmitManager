#!/bin/bash

set -e

EXT_DIR=$(pwd)

git clone https://github.com/alliedmodders/metamod-source --branch "$MMBRANCH" --single-branch "$EXT_DIR/mmsource-$MMBRANCH"
git clone https://github.com/alliedmodders/hl2sdk --branch csgo --single-branch "$EXT_DIR/hl2sdk-csgo"
git clone https://github.com/alliedmodders/sourcemod --recursive --branch "$SMBRANCH" --single-branch "$EXT_DIR/sourcemod-$SMBRANCH"

mkdir -p "$EXT_DIR/build"
pushd "$EXT_DIR/build"
python3 "$EXT_DIR/configure.py" --enable-optimize --mms-path "$EXT_DIR/mmsource-$MMBRANCH" --sm-path "$EXT_DIR/sourcemod-$SMBRANCH" --hl2sdk-root "$EXT_DIR" -s csgo 
ambuild

# might be optional
strip package/addons/sourcemod/extensions/TransmitManager.ext.2.csgo.so

popd