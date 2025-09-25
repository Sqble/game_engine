
set -e

set -x

BUILD_DIR="build"

if [ !k -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

pushd "$BUILD_DIR"

cmake ..

cmake --build . 

./game

popd