#!/bin/sh
set -eu

cd "$(dirname "$(readlink "$0" || echo "$0")")"

COMMAND=$1; shift  # 'init' or 'build' or 'run' or 'lldb' or 'valgrind'
case $COMMAND in
	"init" )
		git submodule update --init --recursive
		./lib/emsdk/emsdk install 3.1.28
		./lib/emsdk/emsdk activate 3.1.28
		cp ./lib/openMVG/src/software/SfM/main_SfMInit_ImageListing.cpp ./main.cpp
		;;
	"build" )
		TARGET=${1:-wasm}       # 'wasm' or 'native'
		BUILD_TYPE=${2:-DEBUG}  # 'DEBUG' or 'RelWithDebInfo'
		CMAKE_OPTION=""
		case $TARGET in
			"wasm" )
				CMAKE_OPTION="-DCMAKE_TOOLCHAIN_FILE='./lib/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake'"
				;;
			"native" )
				CMAKE_OPTION="-DCMAKE_C_COMPILER='clang' -DCMAKE_CXX_COMPILER='clang++'"
				;;
		esac
		DST="./build/${TARGET}/${BUILD_TYPE}"
		cmake \
			-S . -B "${DST}" \
			-DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
			-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
			${CMAKE_OPTION}
		cmake --build ${DST} -j12
		;;
	"run" )
		TARGET=${1:-wasm}       # 'wasm' or 'native'
		BUILD_TYPE=${2:-DEBUG}  # 'DEBUG' or 'RelWithDebInfo'
		DST="./build/${TARGET}/${BUILD_TYPE}"
		case $TARGET in
			"wasm" )
				node --trace-uncaught "${DST}/wasmMVG.js" -i "./lib/ImageDataset_SceauxCastle/images" -o "./build/matches" -c 3 -f 3398
				;;
			"native" )
				"${DST}/wasmMVG" -i "./lib/ImageDataset_SceauxCastle/images" -o "./build/matches" -c 3 -f 3398
				;;
		esac
		;;
	"lldb" )
		TARGET=${1:-wasm}       # 'wasm' or 'native'
		BUILD_TYPE=${2:-DEBUG}  # 'DEBUG' or 'RelWithDebInfo'
		DST="./build/${TARGET}/${BUILD_TYPE}"
		case $TARGET in
			"wasm" )
				;;
			"native" )
				lldb "${DST}/wasmMVG"
				# run -i "./lib/ImageDataset_SceauxCastle/images" -o "./build/matches" -c 3 -f 3398
				;;
		esac
		;;
	"valgrind" )
		TARGET=${1:-wasm}       # 'wasm' or 'native'
		BUILD_TYPE=${2:-DEBUG}  # 'DEBUG' or 'RelWithDebInfo'
		DST="./build/${TARGET}/${BUILD_TYPE}"
		case $TARGET in
			"wasm" )
				;;
			"native" )
				valgrind "${DST}/wasmMVG" -i "./lib/ImageDataset_SceauxCastle/images" -o "./build/matches" -c 3 -f 3398
				;;
		esac
		;;
esac

