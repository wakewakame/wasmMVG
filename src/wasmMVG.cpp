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
 * @param[in] view1               カメラ1の内部パラメータと特徴点 (最低でも13点)
 * @param[in] view2               view1 に対応するカメラ2の内部パラメータと特徴点
 * @param[in] max_iteration_count 最大反復回数
 * @return カメラ1からカメラ2への相対姿勢
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 * @exception 相対姿勢の推定に失敗した場合に std::runtime_error が発生
 */
Pose3 getRelativePose(const View &view1, const View &view2, const size_t max_iteration_count) {
	if (view1.points.rows() != 2) {
		throw std::invalid_argument("view1.points must have 2 rows.");
	}
	if (view2.points.rows() != 2) {
		throw std::invalid_argument("view2.points must have 2 rows.");
	}
	if (view1.points.cols() != view2.points.cols()) {
		throw std::invalid_argument("view1.points and view2.points must have the same number of columns.");
	}

	// 特徴点の歪み補正
	const Mat cam1_points_undisto = columnMap(view1.points, [&](const Vec &p) {
		return view1.intrinsic->get_ud_pixel(p);
	});
	const Mat cam2_points_undisto = columnMap(view2.points, [&](const Vec &p) {
		return view2.intrinsic->get_ud_pixel(p);
	});

	// カメラ位置の推定
	openMVG::sfm::RelativePose_Info relativePose_info;
	if(!openMVG::sfm::robustRelativePose(
		view1.intrinsic.get(), view2.intrinsic.get(), cam1_points_undisto, cam2_points_undisto, relativePose_info,
		{view1.intrinsic->w(), view1.intrinsic->h()}, {view2.intrinsic->w(), view2.intrinsic->h()},
		max_iteration_count
	)) { throw std::runtime_error("Failed to estimate relative pose."); }

	return relativePose_info.relativePose;
}

/**
 * @brief カメラの姿勢を推定
 * @param[in] view      カメラ内部パラメータと特徴点のスクリーン座標
 * @param[in] points_3d view.points に対応する特徴点の現実座標 (3 x N)
 * @return カメラの姿勢
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 * @exception 姿勢の推定に失敗した場合に std::runtime_error が発生
 */
Pose3 getPose(const View &view, const Mat &points_3d) {
	if (view.points.rows() != 2) {
		throw std::invalid_argument("view.points must have 2 rows.");
	}
	if (points_3d.rows() != 3) {
		throw std::invalid_argument("points_3d must have 3 rows.");
	}
	if (view.points.cols() != points_3d.cols()) {
		throw std::invalid_argument("view.points and points_3d must have the same number of columns.");
	}
	const size_t num_points = view.points.cols();

	// 特徴点の歪み補正
	const Mat points_2d_undisto = columnMap(view.points, [&](const Vec &p) {
		return view.intrinsic->get_ud_pixel(p);
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
	if (!openMVG::sfm::SfM_Localizer::Localize(
		openMVG::resection::SolverType::DEFAULT,
		{view.intrinsic->w(), view.intrinsic->h()},
		view.intrinsic.get(),
		resection_data,
		pose
	)) { throw std::runtime_error("Failed to estimate pose."); }

	return pose;
}

/**
 * @brief 特徴点の三次元座標を三角測量
 * @param[in] view1 カメラ1 (内部パラメータ+姿勢) と特徴点のスクリーン座標
 * @param[in] view2 view1 に対応するカメラ2と特徴点のスクリーン座標
 * @return 特徴点の三次元座標 (3 x N)
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 */
Mat triangulation(const PosedView &view1, const PosedView &view2) {
	if (view1.points.rows() != 2) {
		throw std::invalid_argument("view1.points must have 2 rows.");
	}
	if (view2.points.rows() != 2) {
		throw std::invalid_argument("view2.points must have 2 rows.");
	}
	if (view1.points.cols() != view2.points.cols()) {
		throw std::invalid_argument("view1.points and view2.points must have the same number of columns.");
	}
	const size_t num_points = view1.points.cols();

	// 特徴点の歪み補正
	const Mat cam1_points_undisto = columnMap(view1.points, [&](const Vec &p) {
		return view1.camera.intrinsic->get_ud_pixel(p);
	});
	const Mat cam2_points_undisto = columnMap(view2.points, [&](const Vec &p) {
		return view2.camera.intrinsic->get_ud_pixel(p);
	});

	// 点群の三次元座標を推定
	Mat points_3d{3, num_points};
	for(size_t col = 0; col < num_points; col++) {
		const Vec2 x1 = cam1_points_undisto.col(col), x2 = cam2_points_undisto.col(col);
		Vec3 X;
		openMVG::Triangulate2View(
			view1.camera.pose.rotation(), view1.camera.pose.translation() , (*view1.camera.intrinsic)(x1),
			view2.camera.pose.rotation(), view2.camera.pose.translation() , (*view2.camera.intrinsic)(x2),
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
 * @return バンドル調整後のシーン
 * @exception バンドル調整に失敗した場合に std::runtime_error が発生
 */
Scene bundleAdjustment(const Scene &scene) {
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
		throw std::runtime_error("Failed to bundle adjustment.");
	}

	return SfM_DataToScene(sfm_data);
}
