#include "wasmMVG_bindings.h"

#ifdef __EMSCRIPTEN__

Val error(const std::string &description) {
	Val result = Val::object();
	result.set("result", Val("error"));
	result.set("description", Val(description));
	return result;
}
Val ok(const Val &value) {
	Val result = Val::object();
	result.set("result", Val("ok"));
	result.set("value", value);
	return result;
}
Mat valToMat(const Val &val) {
	/*
	 * TODO: 引数のバリデートを行う
	 */
	const size_t cols = val["length"].as<unsigned int>();
	const size_t rows = val[0]["length"].as<unsigned int>();
	Mat ret{rows, cols};
	for(size_t col = 0; col < cols; col++) {
		for(size_t row = 0; row < rows; row++) {
			ret(row, col) = val[col][row].as<double>();
		}
	}
	return ret;
}
Val matToVal(const Mat &mat) {
	Val ret = Val::array();
	for(size_t col = 0; col < mat.cols(); col++) {
		Val row_val = Val::array();
		for(size_t row = 0; row < mat.rows(); row++) {
			row_val.call<Val>("push", Val(mat(row, col)));
		}
		ret.call<Val>("push", row_val);
	}
	return ret;
}
// フラットな Float64Array (列優先 = 点ごとに連続) を rows×N 行列へ変換する。
// 例: rows=2 のとき [x0,y0,x1,y1,...] の各点が行列の 1 列になる。
// emscripten::convertJSArrayToNumberVector は内部で typed_memory_view + set() を使い、
// TypedArray を一括コピーするため、要素ごとの JS↔WASM 往復が発生しない。
Mat valToPoints(const Val &val, const size_t rows) {
	const std::vector<double> buf = emscripten::convertJSArrayToNumberVector<double>(val);
	if (rows == 0 || buf.size() % rows != 0) {
		throw std::invalid_argument("invalid points array length");
	}
	const size_t cols = buf.size() / rows;
	// Mat (openMVG::Mat) は列優先なので、フラット配列をそのまま rows×N 行列として写せる。
	return Eigen::Map<const Mat>(buf.data(), rows, cols);
}
// rows×N 行列を列優先のフラットな Float64Array へ変換する (新しい JS 配列へコピーされる)。
Val pointsToVal(const Mat &mat) {
	return Val::global("Float64Array").new_(
		emscripten::typed_memory_view(static_cast<size_t>(mat.size()), mat.data()));
}
Vec valToVec(const Val &val) {
	/*
	 * TODO: 引数のバリデートを行う
	 */
	const size_t rows = val["length"].as<unsigned int>();
	Vec ret{rows};
	for(size_t row = 0; row < rows; row++) {
		ret(row) = val[row].as<double>();
	}
	return ret;
}
Val vecToVal(const Vec &vec) {
	Val ret = Val::array();
	for(size_t row = 0; row < vec.rows(); row++) {
		ret.call<Val>("push", Val(vec(row)));
	}
	return ret;
}
Intrinsic valToIntrinsic(const Val &val) {
	/*
	 * TODO: 引数のバリデートを行う
	 */
	Intrinsic intrinsic;
	const std::string intrinsic_type = val["type"].as<std::string>();
	if (intrinsic_type == "PINHOLE_CAMERA_RADIAL3") {
		intrinsic.reset(new openMVG::cameras::Pinhole_Intrinsic_Radial_K3 {
			val["width"].as<int>(),     // TODO: 未指定の場合はエラーにする
			val["height"].as<int>(),    // TODO: 未指定の場合はエラーにする
			val["focal"].as<double>(),  // TODO: 未指定の場合は (w+h)/2 にする
			val["ppx"].as<double>(),    // TODO: 未指定の場合は w/2 にする
			val["ppy"].as<double>(),    // TODO: 未指定の場合は h/2 にする
			val["k1"].as<double>(),     // TODO: 未指定の場合は 0 にする
			val["k2"].as<double>(),     // TODO: 未指定の場合は 0 にする
			val["k3"].as<double>()      // TODO: 未指定の場合は 0 にする
		});
	}
	if (!intrinsic) {
		throw std::invalid_argument("Unsupported intrinsic type: " + intrinsic_type);
	}
	return intrinsic;
}
Val intrinsicToVal(const Intrinsic &intrinsic) {
	Val result = Val::object();
	result.set("width", intrinsic->w());
	result.set("height", intrinsic->h());
	if (intrinsic->getType() == openMVG::cameras::PINHOLE_CAMERA_RADIAL3) {
		result.set("type", "PINHOLE_CAMERA_RADIAL3");
		const std::vector<double> params = intrinsic->getParams();
		result.set("focal", params[0]);
		result.set("ppx", params[1]);
		result.set("ppy", params[2]);
		result.set("k1", params[3]);
		result.set("k2", params[4]);
		result.set("k3", params[5]);
	}
	return result;
}
Pose3 valToPose(const Val &val) {
	/*
	 * TODO: 引数のバリデートを行う
	 */
	const Mat rotation = valToMat(val["rotation"]);
	const Vec translation = valToVec(val["translation"]);
	return Pose3{ rotation, - rotation.transpose() * translation };
}
Val poseToVal(const Pose3 &pose) {
	Val result = Val::object();
	result.set("rotation", matToVal(pose.rotation()));
	result.set("translation", vecToVal(pose.translation()));
	return result;
}
Camera valToCamera(const Val &val) {
	/*
	 * TODO: 引数のバリデートを行う
	 */
	const Intrinsic intrinsic = valToIntrinsic(val["intrinsic"]);
	const Pose3 pose = valToPose(val["pose"]);
	return Camera{ intrinsic, pose };
}
Val cameraToVal(const Camera &camera) {
	Val result = Val::object();
	result.set("intrinsic", intrinsicToVal(camera.intrinsic));
	result.set("pose", poseToVal(camera.pose));
	return result;
}
Scene valToScene(const Val &val) {
	/*
	 * TODO: 引数のバリデートを行う
	 */
	const Val symbol_iterator = Val::global("Symbol")["iterator"];
	const Val cameras = Val::global("Object").call<Val>("entries", val["cameras"]);
	const Val points = Val::global("Object").call<Val>("entries", val["points"]);
	Scene scene;

	const size_t num_cameras = cameras["length"].as<unsigned int>();
	for(size_t i = 0; i < num_cameras; i++) {
		const Val camera = cameras[i];
		const size_t key = Val::global("Number")(camera[0]).as<unsigned int>();
		scene.cameras[key] = valToCamera(camera[1]);
	}
	const size_t num_points = points["length"].as<unsigned int>();
	for(size_t i = 0; i < num_points; i++) {
		const Val point = points[i];
		const size_t key = Val::global("Number")(point[0]).as<unsigned int>();
		const Val value = point[1];
		Point &scene_point = scene.points[key];
		scene_point.point = valToVec(value["point"]);
		const Val value_cameras = Val::global("Object").call<Val>("entries", value["cameras"]);
		const size_t num_value_cameras = value_cameras["length"].as<unsigned int>();
		for(size_t j = 0; j < num_value_cameras; j++) {
			const Val value_camera = value_cameras[j];
			const size_t value_camera_key = Val::global("Number")(value_camera[0]).as<unsigned int>();
			scene_point.cameras[value_camera_key] = valToVec(value_camera[1]);
		}
	}

	return scene;
}
Val sceneToVal(const Scene &scene) {
	Val cameras = Val::object();
	for(const std::pair<const size_t, Camera> &camera : scene.cameras) {
		cameras.set(camera.first, cameraToVal(camera.second));
	}

	Val points = Val::object();
	for(const std::pair<const size_t, Point> &point : scene.points) {
		Val point_ = Val::object();
		point_.set("point", vecToVal(point.second.point));
		Val point_cameras_ = Val::object();
		for(const std::pair<const size_t, Vec2> &point_cameras : point.second.cameras) {
			point_cameras_.set(point_cameras.first, vecToVal(point_cameras.second));
		}
		point_.set("cameras", point_cameras_);
		points.set(point.first, point_);
	}

	Val result = Val::object();
	result.set("cameras", cameras);
	result.set("points", points);

	return result;
}

Val getRelativePoseJs(
	const Val &cam1_intrinsic, const Val &cam1_points,
	const Val &cam2_intrinsic, const Val &cam2_points,
	const size_t max_iteration_count
) {
	try {
		const Pose3 pose = getRelativePose(
			View{ valToIntrinsic(cam1_intrinsic), valToPoints(cam1_points, 2) },
			View{ valToIntrinsic(cam2_intrinsic), valToPoints(cam2_points, 2) },
			max_iteration_count
		);
		return ok(poseToVal(pose));
	}
	catch (const std::exception &e) {
		return error(e.what());
	}
}

Val getPoseJs(const Val &intrinsic, const Val &points_2d, const Val &points_3d) {
	try {
		const Pose3 pose = getPose(
			View{ valToIntrinsic(intrinsic), valToPoints(points_2d, 2) },
			valToPoints(points_3d, 3)
		);
		return ok(poseToVal(pose));
	}
	catch (const std::exception &e) {
		return error(e.what());
	}
}

Val triangulationJs(const Val &cam1, const Val &cam1_points, const Val &cam2, const Val &cam2_points) {
	try {
		Mat points_3d = triangulation(
			PosedView{ valToCamera(cam1), valToPoints(cam1_points, 2) },
			PosedView{ valToCamera(cam2), valToPoints(cam2_points, 2) }
		);
		return ok(pointsToVal(points_3d));
	}
	catch (const std::exception &e) {
		return error(e.what());
	}
}

Val bundleAdjustmentJs(const Val &scene) {
	try {
		return ok(sceneToVal(bundleAdjustment(valToScene(scene))));
	}
	catch (const std::exception &e) {
		return error(e.what());
	}
}

#endif  // ifdef __EMSCRIPTEN__
