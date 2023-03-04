#!/bin/bash
# This builds all the imageformat plugins into _external/
set -e # exit on any failure

SCRIPTS_DIR=$(dirname $(readlink -f $0)) # this file's location (/path/to/qimgv/scripts)
SRC_DIR=$(dirname $SCRIPTS_DIR)
EXT_DIR=$SRC_DIR/_external

cd $SRC_DIR
rm -rf "$EXT_DIR"
mkdir "$EXT_DIR"

# ------------------------------------------------------------------------------
echo "BUILDING IMAGEFORMATS"

# qt-jpegxl-image-plugin
cd $EXT_DIR
git clone --depth 1 https://github.com/novomesk/qt-jpegxl-image-plugin.git
cd qt-jpegxl-image-plugin
rm -rf build
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DQT_MAJOR_VERSION=5
ninja -C build

# qt-avif-image-plugin
cd $EXT_DIR
git clone https://github.com/novomesk/qt-avif-image-plugin
cd qt-avif-image-plugin
rm -rf build
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DQT_MAJOR_VERSION=5
ninja -C build

# qt-heif-image-plugin
cd $EXT_DIR
git clone https://github.com/jakar/qt-heif-image-plugin.git
cd qt-heif-image-plugin
rm -rf build
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DQT_MAJOR_VERSION=5
ninja -C build

# QtApng
cd $EXT_DIR
git clone https://github.com/Skycoder42/QtApng.git
cd QtApng
rm -rf build
mkdir build && cd build
qmake.exe ..
make -j4

# qtraw
cd $EXT_DIR
git clone https://gitlab.com/mardy/qtraw
cd qtraw
rm -rf build
mkdir build && cd build
qmake.exe .. DEFINES+="LIBRAW_WIN32_CALLS=1"
make -j4

cd $SRC_DIR
echo "BUILDING DONE"
