#include "wasmMVG.h"

Mat columnMap(const Mat &src, std::function<Vec(const Vec&)> expr) {
	Mat dst{ src.rows(), src.cols() };
	std::transform(src.colwise().begin(), src.colwise().end(), dst.colwise().begin(), expr);
	return dst;
}
SfM_Data sceneToSfM_Data(const Scene& scene) {
	SfM_Data sfm_data;
	for(const std::pair<const size_t, Camera> &camera : scene.cameras) {
		sfm_data.poses[camera.first] = camera.second.pose;
		sfm_data.intrinsics[camera.first] = camera.second.intrinsic;
		sfm_data.views[camera.first] = std::make_shared<openMVG::sfm::View>(
			"", camera.first, camera.first, camera.first,
			camera.second.intrinsic->w(), camera.second.intrinsic->h()
		);
	}
	for(const std::pair<const size_t, Point> &point : scene.points) {
		openMVG::sfm::Observations obs;
		for(const std::pair<const size_t, Vec2> &camera : point.second.cameras) {
			obs[camera.first] = openMVG::sfm::Observation(camera.second, point.first);
		}
		sfm_data.structure[point.first].obs = std::move(obs);
		sfm_data.structure[point.first].X = point.second.point;
	}
	return sfm_data;
}
Scene SfM_DataToScene(const SfM_Data &sfm_data) {
	Scene scene;
	for(const std::pair<const unsigned int, std::shared_ptr<openMVG::sfm::View>> &view : sfm_data.views) {
		Camera &camera = scene.cameras[view.first];
		camera.pose = sfm_data.poses.at(view.second->id_pose);
		camera.intrinsic = sfm_data.intrinsics.at(view.second->id_intrinsic);
	}
	for(const std::pair<const unsigned int, openMVG::sfm::Landmark> &landmark : sfm_data.structure) {
		Point &point = scene.points[landmark.first];
		for(const std::pair<const unsigned int, openMVG::sfm::Observation> &obs : landmark.second.obs) {
			point.cameras[obs.first] = obs.second.x;
		}
		point.point = landmark.second.X;
	}
	return scene;
}

/**
 * @brief wasm 動作確認用の関数
 * @param[in] name 任意の文字列
 * @return         "hello <name>" という文字列
 */
std::string hello(const std::string& name) {
	return "hello " + name;
}

/**
 * @brief 2台のカメラ間の相対姿勢を推定
 * @param[in] cam1_intrinsic      カメラ1のカメラ内部パラメータ
 * @param[in] cam1_points         カメラ1に映る特徴点のスクリーン座標 (最低でも13個)
 * @param[in] cam2_intrinsic      カメラ2のカメラ内部パラメータ
 * @param[in] cam2_points         cam1_points に対応するカメラ2の特徴点のスクリーン座標
 * @param[in] max_iteration_count 最大反復回数
 * @return (カメラ1からカメラ2への相対姿勢, 成功した場合は true)
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 */
std::pair<Pose3, bool> getRelativePose(
	const Intrinsic &cam1_intrinsic, const Mat &cam1_points,
	const Intrinsic &cam2_intrinsic, const Mat &cam2_points,
	const size_t max_iteration_count
) {
	if (cam1_points.rows() != 2) {
		throw std::invalid_argument("cam1_points must have 2 rows.");
	}
	if (cam2_points.rows() != 2) {
		throw std::invalid_argument("cam2_points must have 2 rows.");
	}
	if (cam1_points.cols() != cam2_points.cols()) {
		throw std::invalid_argument("cam1_points and cam2_points must have the same number of columns.");
	}

	// 特徴点の歪み補正
	const Mat cam1_points_undisto = columnMap(cam1_points, [cam1_intrinsic](const Vec &p) {
		return cam1_intrinsic->get_ud_pixel(p);
	});
	const Mat cam2_points_undisto = columnMap(cam2_points, [cam2_intrinsic](const Vec &p) {
		return cam2_intrinsic->get_ud_pixel(p);
	});

	// カメラ位置の推定
	openMVG::sfm::RelativePose_Info relativePose_info;
	if(!openMVG::sfm::robustRelativePose(
		cam1_intrinsic.get(), cam2_intrinsic.get(), cam1_points_undisto, cam2_points_undisto, relativePose_info,
		{cam1_intrinsic->w(), cam1_intrinsic->h()}, {cam2_intrinsic->w(), cam2_intrinsic->h()},
		max_iteration_count
	)) { return { {}, false }; }

	return { relativePose_info.relativePose, true };
}

/**
 * @brief カメラの姿勢を推定
 * @param[in] intrinsic カメラ内部パラメータ
 * @param[in] points_2d カメラに映る特徴点のスクリーン座標
 * @param[in] points_3d カメラに映る特徴点の現実座標
 * @return カメラの姿勢
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 */
Pose3 getPose(const Intrinsic &intrinsic, const Mat &points_2d, const Mat &points_3d) {
	if (points_2d.rows() != 2) {
		throw std::invalid_argument("points_2d must have 2 rows.");
	}
	if (points_3d.rows() != 3) {
		throw std::invalid_argument("points_3d must have 3 rows.");
	}
	if (points_2d.cols() != points_3d.cols()) {
		throw std::invalid_argument("points_2d and points_3d must have the same number of columns.");
	}
	const size_t num_points = points_2d.cols();

	// 特徴点の歪み補正
	const Mat points_2d_undisto = columnMap(points_2d, [intrinsic](const Vec &p) {
		return intrinsic->get_ud_pixel(p);
	});

	openMVG::sfm::Image_Localizer_Match_Data resection_data;
	resection_data.pt2D.resize(2, num_points);
	resection_data.pt3D.resize(3, num_points);
	for(size_t col = 0; col < num_points; col++) {
		resection_data.pt2D.col(col) = points_2d_undisto.col(col);
		resection_data.pt3D.col(col) = points_3d.col(col);
	}

	// カメラ位置の推定
	Pose3 pose;
	openMVG::sfm::SfM_Localizer::Localize(
		openMVG::resection::SolverType::DEFAULT,
		{intrinsic->w(), intrinsic->h()},
		intrinsic.get(),
		resection_data,
		pose
	);

	return pose;
}

/**
 * @brief 特徴点の三次元座標を三角測量
 * @param[in] cam1_intrinsic カメラ1のカメラ内部パラメータと姿勢
 * @param[in] cam1_points    カメラ1に映る特徴点のスクリーン座標
 * @param[in] cam2_intrinsic カメラ2のカメラ内部パラメータと姿勢
 * @param[in] cam2_points    cam1_points に対応するカメラ2の特徴点のスクリーン座標
 * @return 特徴点の三次元座標
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 */
Mat triangulation(const Camera &cam1, const Mat &cam1_points, const Camera &cam2, const Mat &cam2_points) {
	if (cam1_points.rows() != 2) {
		throw std::invalid_argument("cam1_points must have 2 rows.");
	}
	if (cam2_points.rows() != 2) {
		throw std::invalid_argument("cam2_points must have 2 rows.");
	}
	if (cam1_points.cols() != cam2_points.cols()) {
		throw std::invalid_argument("cam1_points and cam2_points must have the same number of columns.");
	}
	const size_t num_points = cam1_points.cols();

	// 特徴点の歪み補正
	const Mat cam1_points_undisto = columnMap(cam1_points, [cam1](const Vec &p) {
		return cam1.intrinsic->get_ud_pixel(p);
	});
	const Mat cam2_points_undisto = columnMap(cam2_points, [cam2](const Vec &p) {
		return cam2.intrinsic->get_ud_pixel(p);
	});

	// 点群の三次元座標を推定
	Mat points_3d{3, num_points};
	for(size_t col = 0; col < num_points; col++) {
		const Vec2 x1 = cam1_points_undisto.col(col), x2 = cam2_points_undisto.col(col);
		Vec3 X;
		openMVG::Triangulate2View(
			cam1.pose.rotation(), cam1.pose.translation() , (*cam1.intrinsic)(x1),
			cam2.pose.rotation(), cam2.pose.translation() , (*cam2.intrinsic)(x2),
			X,
			openMVG::ETriangulationMethod::DEFAULT
		);
		points_3d.col(col) = X;
	}

	return points_3d;
}

/**
 * @brief バンドル調整
 * @param[in] scene バンドル調整を行うシーン
 * @return (バンドル調整後のシーン, 成功した場合は true)
 */
std::pair<Scene, bool> bundleAdjustment(const Scene &scene) {
	SfM_Data sfm_data = sceneToSfM_Data(scene);

	const openMVG::sfm::Optimize_Options ba_refine_options(
		openMVG::cameras::Intrinsic_Parameter_Type::ADJUST_ALL,
		openMVG::sfm::Extrinsic_Parameter_Type::ADJUST_ALL,
		openMVG::sfm::Structure_Parameter_Type::ADJUST_ALL,
		openMVG::sfm::Control_Point_Parameter(),
		false
	);
	openMVG::sfm::Bundle_Adjustment_Ceres bundle_adjustment_obj;
	if (!bundle_adjustment_obj.Adjust(sfm_data, ba_refine_options)) {
		return { {}, false };
	}

	return { SfM_DataToScene(sfm_data), true };
}

#ifdef __EMSCRIPTEN__

Val error(const std::string &description) {
	Val result = Val::object();
	result.set("result", Val("error"));
	result.set("description", Val(description));
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
		const std::pair<Pose3, bool> pose = getRelativePose(
			valToIntrinsic(cam1_intrinsic), valToMat(cam1_points),
			valToIntrinsic(cam2_intrinsic), valToMat(cam2_points),
			max_iteration_count
		);
		if (!pose.second) { return error("Failed to estimate pose."); }
		return poseToVal(pose.first);
	} 
	catch (const std::invalid_argument &e) {
		return error(e.what());
	}

	return error("Unexpected error.");
}

Val getPoseJs(const Val &intrinsic, const Val &points_2d, const Val &points_3d) {
	try {
		const Pose3 pose = getPose(valToIntrinsic(intrinsic), valToMat(points_2d), valToMat(points_3d));
		return poseToVal(pose);
	} 
	catch (const std::invalid_argument &e) {
		return error(e.what());
	}

	return error("Unexpected error.");
}

Val triangulationJs(const Val &cam1, const Val &cam1_points, const Val &cam2, const Val &cam2_points) {
	try {
		Mat points_3d = triangulation(
			valToCamera(cam1), valToMat(cam1_points),
			valToCamera(cam2), valToMat(cam2_points)
		);
		return matToVal(points_3d);
	}
	catch (const std::invalid_argument &e) {
		return error(e.what());
	}

	return error("Unexpected error.");
}

Val bundleAdjustmentJs(const Val &scene) {
	const std::pair<Scene, bool> result = bundleAdjustment(valToScene(scene));
	if (!result.second) { return error("Failed to bundle adjustment."); }
	return sceneToVal(result.first);
}

#endif  // ifdef __EMSCRIPTEN__
