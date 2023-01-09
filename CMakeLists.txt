cmake_minimum_required(VERSION 3.12)
project(wasmMVG)

# openMVG のビルド、インストール
include(ExternalProject)
string(TOLOWER "openMVG" OPENMVG_TARGET_NAME)
set(OPENMVG_DIR "${PROJECT_BINARY_DIR}/3rdparty/${OPENMVG_TARGET_NAME}")
ExternalProject_Add(
	"${OPENMVG_TARGET_NAME}"
	URL "${CMAKE_CURRENT_SOURCE_DIR}/lib/openMVG"
	SOURCE_SUBDIR "src"
	PREFIX "${OPENMVG_DIR}"
	SOURCE_DIR "${OPENMVG_DIR}/source"
	BINARY_DIR "${OPENMVG_DIR}/build"
	CMAKE_ARGS
		-DCMAKE_INSTALL_PREFIX=${OPENMVG_DIR}/install
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

		# openMVG でビルドする対象
		-DOpenMVG_BUILD_DOC=OFF
		-DOpenMVG_BUILD_EXAMPLES=OFF
		-DOpenMVG_BUILD_SOFTWARES=OFF
		-DOpenMVG_BUILD_GUI_SOFTWARES=OFF

		# openMVG 内の Find*.cmake が見つかるようにする
		-DCMAKE_FIND_ROOT_PATH=${OPENMVG_DIR}/source

		# openMVG に内包されるライブラリを利用するため、一部の find_package を無効化
		-DCMAKE_DISABLE_FIND_PACKAGE_JPEG=ON
		-DCMAKE_DISABLE_FIND_PACKAGE_PNG=ON
		-DCMAKE_DISABLE_FIND_PACKAGE_TIFF=ON
		-DCMAKE_DISABLE_FIND_PACKAGE_cereal=ON
		-DCMAKE_DISABLE_FIND_PACKAGE_Eigen3=ON
		-DCMAKE_DISABLE_FIND_PACKAGE_Ceres=ON

		# emscripten 用オプション
		-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
		-DBUILD_SHARED_LIBS=OFF
		-DM_LIBRARY=""
		-DOpenMVG_USE_OPENMP=OFF

		# 以下のオプションを指定しないと初回ビルドが失敗する (2回目以降は成功する)
		# https://github.com/openMVG/openMVG/issues/1117
		-DINCLUDE_INSTALL_DIR=${OPENMVG_DIR}/install/include/openMVG/third_party/eigen
)

# wasmMVG の作成
add_executable(wasmMVG main.cpp)
target_compile_features(wasmMVG PUBLIC cxx_std_11)  # C++ のバージョンを openMVG に合わせる
add_dependencies(wasmMVG ${OPENMVG_TARGET_NAME})
target_include_directories(wasmMVG PRIVATE
	"${OPENMVG_DIR}/install/include"
	"${OPENMVG_DIR}/install/include/openMVG"
	"${OPENMVG_DIR}/install/include/openMVG/third_party/eigen"
)
target_link_directories(wasmMVG PRIVATE "${OPENMVG_DIR}/install/lib")
# 以下は ${OPENMVG_DIR}/install/lib に生成された lib*.a ファイルを並べている
target_link_libraries(wasmMVG PRIVATE
	lib_clp
	lib_CoinUtils
	lib_Osi
	lib_OsiClpSolver
	"openMVG_ceres$<$<CONFIG:Debug>:-debug>"
	openMVG_exif
	openMVG_easyexif
	openMVG_fast
	openMVG_features
	openMVG_geometry
	openMVG_image
	openMVG_jpeg
	openMVG_kvld
	openMVG_linearProgramming
	openMVG_lInftyComputerVision
	openMVG_matching
	openMVG_matching_image_collection
	openMVG_multiview
	openMVG_numeric
	openMVG_png
	openMVG_robust_estimation
	openMVG_sfm
	openMVG_stlplus
	openMVG_system
	openMVG_tiff
	openMVG_zlib
	vlsift
)

# emscripten 用のオプション
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
	target_compile_options(wasmMVG PRIVATE
		-fwasm-exceptions                 # 例外のキャッチを有効化する
	)
	target_link_options(wasmMVG PRIVATE
		-fwasm-exceptions                 # 例外のキャッチを有効化する
		"SHELL:-s NODERAWFS=1"            # ファイルの読み書きを可能にする
		"SHELL:-s ALLOW_MEMORY_GROWTH=1"  # メモリ不足時の自動確保を有効化する
	)
endif()

