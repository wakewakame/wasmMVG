#include <iostream>
#include "openMVG/sfm/sfm.hpp"
#include "openMVG/cameras/cameras.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

openMVG::Mat valToMat(const emscripten::val &val) {
	const size_t cols = val["length"].as<unsigned int>();
	const size_t rows = val[0]["length"].as<unsigned int>();
	openMVG::Mat ret{rows, cols};
	for(size_t col = 0; col < cols; col++) {
		for(size_t row = 0; row < rows; row++) {
			ret(row, col) = val[col][row].as<double>();
		}
	}
	return ret;
}
emscripten::val matToVal(const openMVG::Mat &mat) {
	emscripten::val ret = emscripten::val::array();
	for(size_t col = 0; col < mat.cols(); col++) {
		emscripten::val row_val = emscripten::val::array();
		for(size_t row = 0; row < mat.rows(); row++) {
			row_val.call<emscripten::val>("push", emscripten::val(mat(row, col)));
		}
		ret.call<emscripten::val>("push", row_val);
	}
	return ret;
}

emscripten::val getRelativePose(
	const emscripten::val &img1_cam, const emscripten::val &img1_points,
	const emscripten::val &img2_cam, const emscripten::val &img2_points
) {
	openMVG::cameras::Pinhole_Intrinsic_Radial_K3 img1_cam_{
		img1_cam["w"].as<int>(),
		img1_cam["h"].as<int>(),
		img1_cam["focal"].as<double>(),
		img1_cam["ppx"].as<double>(),
		img1_cam["ppy"].as<double>(),
		img1_cam["k1"].as<double>(),
		img1_cam["k2"].as<double>(),
		img1_cam["k3"].as<double>()
	};
	openMVG::cameras::Pinhole_Intrinsic_Radial_K3 img2_cam_{
		img2_cam["w"].as<int>(),
		img2_cam["h"].as<int>(),
		img2_cam["focal"].as<double>(),
		img2_cam["ppx"].as<double>(),
		img2_cam["ppy"].as<double>(),
		img2_cam["k1"].as<double>(),
		img2_cam["k2"].as<double>(),
		img2_cam["k3"].as<double>()
	};

	openMVG::Mat img1_points_ = valToMat(img1_points);
	openMVG::Mat img2_points_ = valToMat(img2_points);

	openMVG::sfm::RelativePose_Info relativePose_info;
	if(!robustRelativePose(
		&img1_cam_, &img2_cam_, img1_points_, img2_points_, relativePose_info,
		{img1_cam_.w(), img1_cam_.h()}, {img2_cam_.w(), img2_cam_.h()},
		4096
	)) {
		return emscripten::val::null();
	}

	const emscripten::val rotation = matToVal(relativePose_info.relativePose.rotation());
	const emscripten::val center = matToVal(relativePose_info.relativePose.center());
	emscripten::val result = emscripten::val::object();
	result.set("rotation", rotation);
	result.set("center", center);
	return result;
}

EMSCRIPTEN_BINDINGS(my_class_example) {
	emscripten::function("getRelativePose", &getRelativePose);
}
