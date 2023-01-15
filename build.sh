#!/bin/sh
set -eu

cd "$(dirname "$(readlink "$0" || echo "$0")")"

COMMAND=$1; shift  # 'init' or 'build' or 'run' or 'lldb' or 'valgrind'
case $COMMAND in
	"init" )
		git submodule update --init --recursive
		./lib/emsdk/emsdk install 3.1.28
		./lib/emsdk/emsdk activate 3.1.28
		mkdir -p examples
		cp ./lib/openMVG/src/software/SfM/main_SfMInit_ImageListing.cpp ./examples/
		cp ./lib/openMVG/src/software/SfM/main_ComputeFeatures.cpp      ./examples/
		cp ./lib/openMVG/src/software/SfM/main_ComputeMatches.cpp       ./examples/
		cp ./lib/openMVG/src/software/SfM/main_GeometricFilter.cpp      ./examples/
		cp ./lib/openMVG/src/software/SfM/main_SfM.cpp                  ./examples/
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
				RESULT_DIR="./build"
				echo "step 1: load all images"
				node --trace-uncaught "${DST}/main_SfMInit_ImageListing.js" \
					-i "./lib/ImageDataset_SceauxCastle/images" \
					-o "${RESULT_DIR}/matches" \
					-c "3" \
					-f 3398
					#-k "${CALIB_MATRIX}"
				echo "step 2: feature detection"
				node --trace-uncaught "${DST}/main_ComputeFeatures.js" \
					-i "${RESULT_DIR}/matches/sfm_data.json" \
					-o "${RESULT_DIR}/matches" \
					-m AKAZE_FLOAT \
					-f 1
				echo "step 3: feature matching"
				node --trace-uncaught "${DST}/main_ComputeMatches.js" \
					-i "${RESULT_DIR}/matches/sfm_data.json" \
					-o "${RESULT_DIR}/matches/matches.putative.bin" \
					-f 1 \
					-n ANNL2
				echo "step 4: feature matching filter"
				node --trace-uncaught "${DST}/main_GeometricFilter.js" \
					-i "${RESULT_DIR}/matches/sfm_data.json" \
					-m "${RESULT_DIR}/matches/matches.putative.bin" \
					-g f \
					-o "${RESULT_DIR}/matches/matches.f.bin" \
				echo "step 5: struction from motion"
				node --trace-uncaught "${DST}/main_SfM.js" \
					-s "INCREMENTAL" \
					-i "${RESULT_DIR}/matches/sfm_data.json" \
					-m "${RESULT_DIR}/matches" \
					-o "${RESULT_DIR}/sparse_model"
				#echo "step 6: compute color of the structure"
				#node --trace-uncaught "${DST}/main_ComputeSfM_DataColor.js" \
				#	-i "${RESULT_DIR}/sparse_model/sfm_data.bin" \
				#	-o "${RESULT_DIR}/sparse_model/colorized.ply"
				#echo "step 7: generate sparse point cloud"
				#node --trace-uncaught "${DST}/main_ComputeStructureFromKnownPoses.js" \
				#	-i "${RESULT_DIR}/sparse_model/sfm_data.bin" \
				#	-m "${RESULT_DIR}/matches" \
				#	-o "${RESULT_DIR}/sparse_model/robust.ply"
				#echo "step 8: convert from OpenMVG format to OpenMVS format"
				#node --trace-uncaught "${DST}/main_openMVG2openMVS.js" \
				#	-i "${RESULT_DIR}/sparse_model/sfm_data.bin" \
				#	-o "${RESULT_DIR}/dense_model/scene.mvs" \
				#	-d "${RESULT_DIR}/dense_model"
				;;
			"native" )
				RESULT_DIR="./build"
				echo "step 1: load all images"
				"${DST}/main_SfMInit_ImageListing" \
					-i "./lib/ImageDataset_SceauxCastle/images" \
					-o "${RESULT_DIR}/matches" \
					-c "3" \
					-f 3398
					#-k "${CALIB_MATRIX}"
				echo "step 2: feature detection"
				"${DST}/main_ComputeFeatures" \
					-i "${RESULT_DIR}/matches/sfm_dataon" \
					-o "${RESULT_DIR}/matches" \
					-m AKAZE_FLOAT \
					-f 1
				echo "step 3: feature matching"
				"${DST}/main_ComputeMatches" \
					-i "${RESULT_DIR}/matches/sfm_dataon" \
					-o "${RESULT_DIR}/matches/matches.putative.bin" \
					-f 1 \
					-n ANNL2
				echo "step 4: feature matching filter"
				"${DST}/main_GeometricFilter" \
					-i "${RESULT_DIR}/matches/sfm_dataon" \
					-m "${RESULT_DIR}/matches/matches.putative.bin" \
					-g f \
					-o "${RESULT_DIR}/matches/matches.f.bin" \
				echo "step 5: struction from motion"
				"${DST}/main_SfM" \
					-s "INCREMENTAL" \
					-i "${RESULT_DIR}/matches/sfm_dataon" \
					-m "${RESULT_DIR}/matches" \
					-o "${RESULT_DIR}/sparse_model"
				#echo "step 6: compute color of the structure"
				#"${DST}/main_ComputeSfM_DataColor" \
				#	-i "${RESULT_DIR}/sparse_model/sfm_data.bin" \
				#	-o "${RESULT_DIR}/sparse_model/colorized.ply"
				#echo "step 7: generate sparse point cloud"
				#"${DST}/main_ComputeStructureFromKnownPoses" \
				#	-i "${RESULT_DIR}/sparse_model/sfm_data.bin" \
				#	-m "${RESULT_DIR}/matches" \
				#	-o "${RESULT_DIR}/sparse_model/robust.ply"
				#echo "step 8: convert from OpenMVG format to OpenMVS format"
				#"${DST}/main_openMVG2openMVS" \
				#	-i "${RESULT_DIR}/sparse_model/sfm_data.bin" \
				#	-o "${RESULT_DIR}/dense_model/scene.mvs" \
				#	-d "${RESULT_DIR}/dense_model"
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
				lldb "${DST}/main"
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
				valgrind "${DST}/main"
				;;
		esac
		;;
esac

