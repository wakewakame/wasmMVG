#ifndef WASM_MVG
#define WASM_MVG

#include "openMVG/sfm/sfm.hpp"
#include "openMVG/cameras/cameras.hpp"
#include "openMVG/multiview/triangulation.hpp"

using Mat = openMVG::Mat;
using Vec = openMVG::Vec;
using Vec2 = openMVG::Vec2;
using Vec3 = openMVG::Vec3;
using SfM_Data = openMVG::sfm::SfM_Data;
using Intrinsic = std::shared_ptr<openMVG::cameras::IntrinsicBase>;
using Pose3 = openMVG::geometry::Pose3;
struct Camera {
	Intrinsic intrinsic;
	Pose3 pose;
};
struct Point {
	Vec3 point;
	std::map<size_t, Vec2> cameras;
};
struct Scene {
	std::map<size_t, Camera> cameras;
	std::map<size_t, Point> points;
};
Mat columnMap(const Mat &src, std::function<Vec(const Vec&)> expr);
SfM_Data sceneToSfM_Data(const Scene& scene);
Scene SfM_DataToScene(const SfM_Data &sfm_data);

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
);

/**
 * @brief カメラの姿勢を推定
 * @param[in] intrinsic カメラ内部パラメータ
 * @param[in] points_2d カメラに映る特徴点のスクリーン座標
 * @param[in] points_3d カメラに映る特徴点の現実座標
 * @return カメラの姿勢
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 */
Pose3 getPose(const Intrinsic &intrinsic, const Mat &points_2d, const Mat &points_3d);

/**
 * @brief 特徴点の三次元座標を三角測量
 * @param[in] cam1_intrinsic カメラ1のカメラ内部パラメータと姿勢
 * @param[in] cam1_points    カメラ1に映る特徴点のスクリーン座標
 * @param[in] cam2_intrinsic カメラ2のカメラ内部パラメータと姿勢
 * @param[in] cam2_points    cam1_points に対応するカメラ2の特徴点のスクリーン座標
 * @return 特徴点の三次元座標
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 */
Mat triangulation(const Camera &cam1, const Mat &cam1_points, const Camera &cam2, const Mat &cam2_points);

/**
 * @brief バンドル調整
 * @param[in] scene バンドル調整を行うシーン
 * @return (バンドル調整後のシーン, 成功した場合は true)
 */
std::pair<Scene, bool> bundleAdjustment(const Scene &scene);

#ifdef __EMSCRIPTEN__

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

using Val = emscripten::val;
Val error(const std::string &description);
Mat valToMat(const Val &val);
Val matToVal(const Mat &mat);
Vec valToVec(const Val &val);
Val vecToVal(const Vec &vec);
Intrinsic valToIntrinsic(const Val &val);
Val intrinsicToVal(const Intrinsic &intrinsic);
Pose3 valToPose(const Val &val);
Val poseToVal(const Pose3 &pose);
Camera valToCamera(const Val &val);
Val cameraToVal(const Camera &camera);
Scene valToScene(const Val &val);
Val sceneToVal(const Scene &scene);

Val getRelativePoseJs(
	const Val &cam1_intrinsic, const Val &cam1_points,
	const Val &cam2_intrinsic, const Val &cam2_points,
	const size_t max_iteration_count
);
Val getPoseJs(const Val &intrinsic, const Val &points_2d, const Val &points_3d);
Val triangulationJs(const Val &cam1, const Val &cam1_points, const Val &cam2, const Val &cam2_points);
Val bundleAdjustmentJs(const Val &scene);

#endif  // ifdef __EMSCRIPTEN__
#endif  // ifndef WASM_MVG
