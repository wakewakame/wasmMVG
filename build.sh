#!/bin/bash
set -eu

cd "$(dirname "$(readlink "$0" || echo "$0")")"
SCRIPT_DIR="$(pwd)"

COMMAND=$1; shift  # 'init' or 'build' or 'run' or 'lldb' or 'valgrind'
case $COMMAND in
	"init" )
		git submodule update --init --recursive
		./lib/emsdk/emsdk install 3.1.28
		./lib/emsdk/emsdk activate 3.1.28
		mkdir -p examples
		EXAMPLES=(
			"main_SfMInit_ImageListing"
			"main_ComputeFeatures"
			"main_ComputeMatches"
			"main_GeometricFilter"
			"main_SfM"
		)
		for EXAMPLE in "${EXAMPLES[@]}"; do
			cat "./lib/openMVG/src/software/SfM/${EXAMPLE}.cpp" \
				| sed "s/int main/int ${EXAMPLE}/" \
				> "./examples/${EXAMPLE}.h"
		done
		;;
	"build" )
		TARGET=${1:-wasm}       # 'wasm' or 'native'
		BUILD_TYPE=${2:-DEBUG}  # 'DEBUG' or 'RelWithDebInfo'
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
					-i "${RESULT_DIR}/matches/sfm_data.json" \
					-o "${RESULT_DIR}/matches" \
					-m AKAZE_FLOAT \
					-f 1
				echo "step 3: feature matching"
				"${DST}/main_ComputeMatches" \
					-i "${RESULT_DIR}/matches/sfm_data.json" \
					-o "${RESULT_DIR}/matches/matches.putative.bin" \
					-f 1 \
					-n ANNL2
				echo "step 4: feature matching filter"
				"${DST}/main_GeometricFilter" \
					-i "${RESULT_DIR}/matches/sfm_data.json" \
					-m "${RESULT_DIR}/matches/matches.putative.bin" \
					-g f \
					-o "${RESULT_DIR}/matches/matches.f.bin" \
				echo "step 5: struction from motion"
				"${DST}/main_SfM" \
					-s "INCREMENTAL" \
					-i "${RESULT_DIR}/matches/sfm_data.json" \
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
				lldb "${DST}/main_SfMInit_ImageListing"
				# run -i "./lib/ImageDataset_SceauxCastle/images" -o "./build/matches" -c "3" -f 3398
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
				RESULT_DIR="./build"
				valgrind "${DST}/main_SfMInit_ImageListing" \
					-i "./lib/ImageDataset_SceauxCastle/images" \
					-o "${RESULT_DIR}/matches" \
					-c "3" \
					-f 3398
				;;
		esac
		;;
esac

