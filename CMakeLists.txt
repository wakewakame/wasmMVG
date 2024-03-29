cmake_minimum_required(VERSION 3.14)
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
		-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
		-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}

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
		-DOpenMVG_BUILD_SHARED=OFF
		-DM_LIBRARY=""
		-DOpenMVG_USE_OPENMP=OFF
		-DOPENMP=OFF

		# 以下のオプションを指定しないと初回ビルドが失敗する (2回目以降は成功する)
		# https://github.com/openMVG/openMVG/issues/1117
		-DINCLUDE_INSTALL_DIR=${OPENMVG_DIR}/install/include/openMVG/third_party/eigen
)

# OpenMVG はコンパイルオプションによって構造体のメモリレイアウトが変わるが、
# コンパイル時の型チェックでレイアウトは検査されないので不正なメモリアクセスにより segfault する。
# 以下はそのエラーを回避するためのオプション。
# 参照: https://github.com/openMVG/openMVG/blob/e1bbfe801986cd7171f36443a1573b0f69f3702d/src/CMakeLists.txt#L106
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_definitions(OPENMVG_STD_UNORDERED_MAP)
endif()

# 実行ファイルを作成するマクロ
macro(make_app)
	set(oneValueArgs APP_NAME)
	set(multiValueArgs SOURCES LIBRARIES)
	cmake_parse_arguments(ARG "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	add_executable(${ARG_APP_NAME}
		"${CMAKE_CURRENT_SOURCE_DIR}/src/wasmMVG.cpp"
		${ARG_SOURCES}
	)
	target_compile_features(${ARG_APP_NAME} PUBLIC cxx_std_11)  # C++ のバージョンを openMVG に合わせる
	add_dependencies(${ARG_APP_NAME} ${OPENMVG_TARGET_NAME})
	target_include_directories(${ARG_APP_NAME} PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${OPENMVG_DIR}/install/include"
		"${OPENMVG_DIR}/install/include/openMVG"
		"${OPENMVG_DIR}/install/include/openMVG/third_party"
		"${OPENMVG_DIR}/install/include/openMVG/third_party/eigen"
		"${OPENMVG_DIR}/install/include/openMVG/third_party/lemon"
		"${OPENMVG_DIR}/install/include/openMVG_dependencies"
		"${OPENMVG_DIR}/install/include/openMVG_dependencies/cereal/include"
	)
	target_link_directories(${ARG_APP_NAME} PRIVATE "${OPENMVG_DIR}/install/lib")
	# 以下は ${OPENMVG_DIR}/install/lib に生成された lib*.a ファイルを並べている。
	# なお、依存されているライブラリは依存しているライブラリよりも下に書く必要がある。
	target_link_libraries(${ARG_APP_NAME} PRIVATE
		${ARG_LIBRARIES}
		openMVG_exif
		openMVG_easyexif
		openMVG_sfm
		openMVG_kvld
		openMVG_matching_image_collection
		openMVG_matching
		openMVG_features
		openMVG_geometry
		openMVG_image
		openMVG_lInftyComputerVision
		openMVG_linearProgramming
		openMVG_multiview
		openMVG_robust_estimation
		openMVG_jpeg
		openMVG_tiff
		openMVG_png
		openMVG_zlib
		openMVG_numeric
		openMVG_fast
		"openMVG_ceres$<$<CONFIG:Debug>:-debug>"
		openMVG_system
		openMVG_stlplus
		vlsift
		lib_clp
		lib_CoinUtils
		lib_OsiClpSolver
		lib_Osi
	)

	# emscripten 用のオプション
	if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
		target_compile_options(${ARG_APP_NAME} PRIVATE
			-fwasm-exceptions                 # 例外のキャッチを有効化する
			#-gseparate-dwarf                  # DWARF デバッグ情報を出力する
			-Wall -Werror
			-Wno-bitwise-instead-of-logical
			-Wno-unused-but-set-variable
		)
		target_link_options(${ARG_APP_NAME} PRIVATE
			-fwasm-exceptions                 # 例外のキャッチを有効化する
			-lembind

			# ブラウザで wasm を動かすときは以下のオプションを削除しないと
			# printf や std::cout が表示されなくなる
			#"SHELL:-s NODERAWFS=1"            # ファイルの読み書きを可能にする

			"SHELL:-s ALLOW_MEMORY_GROWTH=1"  # メモリ不足時の自動確保を有効化する
			"SHELL:-s NO_EXIT_RUNTIME=1"
			"SHELL:-s EXPORT_ES6=1"
			"SHELL:-s MODULARIZE=1"
			"SHELL:-s EXPORT_NAME=wasmMVG"
			"SHELL:-s EXPORTED_RUNTIME_METHODS=cwrap,ccall"
			"SHELL:-s EXPORTED_FUNCTIONS=['_malloc','_free']"
		)
	else()
		target_compile_options(${ARG_APP_NAME} PRIVATE
			-Wall -Werror
			-Wno-bitwise-instead-of-logical
			-Wno-unused-but-set-variable
		)
	endif()
endmacro()

# 実行ファイル作成
make_app(APP_NAME main SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# テストの追加
# 参考元: https://google.github.io/googletest/quickstart-cmake.html
if (NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
	include(FetchContent)
	FetchContent_Declare(
	  googletest
	  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
	)
	# For Windows: Prevent overriding the parent project's compiler/linker settings
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)
	enable_testing()
	make_app(
		APP_NAME wasmMVG_test
		SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp"
		LIBRARIES GTest::gtest_main
	)
	include(GoogleTest)
	gtest_discover_tests(wasmMVG_test)
endif()
