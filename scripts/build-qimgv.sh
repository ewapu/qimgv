#!/bin/bash
# This installs dependencies and builds qimgv 

#CFL='-ffunction-sections -fdata-sections -march=native -mtune=native -O3 -pipe'
CFL='-ffunction-sections -fdata-sections -O3 -pipe'
LDFL='-Wl,--gc-sections'

SCRIPTS_DIR=$(dirname $(readlink -f $0)) # this file's location (/path/to/qimgv/scripts)
SRC_DIR=$(dirname $SCRIPTS_DIR)
BUILD_DIR=$SRC_DIR/build

cd $SRC_DIR
# ------------------------------------------------------------------------------
echo "INSTALLING MSYS2 BUILD DEPS"

if [[ -n "$RUNNER_TEMP" ]]; then
    MSYS_DEPS=$(cat $SCRIPTS_DIR/msys2-build-deps.txt | sed 's/\n/ /')
    pacman -S $MSYS_DEPS --noconfirm --needed
fi

# ------------------------------------------------------------------------------
echo "BUILDING"
# rm -rf $BUILD_DIR
sed -i 's|opencv4/||' $SRC_DIR/qimgv/3rdparty/QtOpenCV/cvmatandqimage.{h,cpp}
cmake -S $SRC_DIR -B $BUILD_DIR -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DOPENCV_SUPPORT=ON \
    -DVIDEO_SUPPORT=ON \
    -DCMAKE_CXX_FLAGS="$CFL" -DCMAKE_EXE_LINKER_FLAGS="$LDFL" \
ninja -C $BUILD_DIR

echo "BUILDING DONE"
