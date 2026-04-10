#!/bin/bash

set -e

BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

install_deps() {
    echo "Installing dependencies..."
    sudo pacman -S --needed cmake make gcc glfw-x11 glm mesa libepoxy
}

configure_and_build() {
    pushd "$BUILD_DIR"

    cmake .. -DCMAKE_BUILD_TYPE=Release "$@"
    cmake --build . -j$(nproc)

    popd
}

run_tests() {
    pushd "$BUILD_DIR"

    ctest --output-on-failure

    popd
}

run_game() {
    ./build/game
}

case "${1:-run}" in
    deps)
        install_deps
        ;;
    build)
        configure_and_build
        ;;
    test)
        configure_and_build -DBUILD_TESTING=ON
        run_tests
        ;;
    run)
        configure_and_build
        run_game
        ;;
    clean)
        rm -rf "$BUILD_DIR"
        ;;
    *)
        echo "Usage: $0 [deps|build|test|run|clean]"
        exit 1
        ;;
esac
