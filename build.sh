#!/bin/bash
set -eu

cd "$(dirname "$(readlink "$0" || echo "$0")")"
SCRIPT_DIR="$(pwd)"

COMMAND=$1; shift  # 'init' or 'build' or 'test'
case $COMMAND in
	"init" )
		git submodule update --init --recursive
		./lib/emsdk/emsdk install 3.1.28
		./lib/emsdk/emsdk activate 3.1.28
		;;
	"build" )
		TARGET=${1:-wasm}  # 'wasm' or 'native'
		BUILD_TYPE=${2:-RelWithDebInfo}  # 'DEBUG' or 'RelWithDebInfo'
		CMAKE_COMMAND=()
		case $TARGET in
			"all" )
				./build.sh build native DEBUG
				./build.sh build native RELEASE
				./build.sh build wasm DEBUG
				./build.sh build wasm RELEASE
				exit 0
				;;
			"wasm" )
				CMAKE_COMMAND=("./lib/emsdk/upstream/emscripten/emcmake" "cmake")
				;;
			"native" )
				CMAKE_COMMAND=("cmake" "-DCMAKE_C_COMPILER='clang'" "-DCMAKE_CXX_COMPILER='clang++'")
				;;
		esac
		DST="./build/${TARGET}/${BUILD_TYPE}"
		${CMAKE_COMMAND[@]} \
			-S . -B "${DST}" \
			-DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
			-DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        cat "./build/${TARGET}/${BUILD_TYPE}/compile_commands.json" \
            | jq ". |= map(select(.command != null).command |= sub(\" -gseparate-dwarf\"; \"\") + \" -I ${SCRIPT_DIR}/lib/emsdk/upstream/emscripten/cache/sysroot/include\")" \
            > ./compile_commands.json
		cmake --build ${DST} -j12
		;;
	"test" )
		./build.sh build native DEBUG
		cd ./build/native/DEBUG
		ctest
		;;
esac

