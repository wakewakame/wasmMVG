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
// 姿勢が未知のビュー (姿勢推定の入力)。
// カメラ内部パラメータと、そのカメラに映る特徴点のスクリーン座標 (2 x N) を持つ。
struct View {
	Intrinsic intrinsic;
	Mat points;
};
// 姿勢が既知のビュー (三角測量の入力)。
// カメラ (内部パラメータ + 姿勢) と、そのカメラに映る特徴点のスクリーン座標 (2 x N) を持つ。
struct PosedView {
	Camera camera;
	Mat points;
};
Mat columnMap(const Mat &src, std::function<Vec(const Vec&)> expr);
SfM_Data sceneToSfM_Data(const Scene& scene);
Scene SfM_DataToScene(const SfM_Data &sfm_data);

/**
 * @brief wasm 動作確認用の関数
 * @param[in] name 任意の文字列
 * @return         "hello <name>" という文字列
 */
std::string hello(const std::string &name);

/**
 * @brief 2台のカメラ間の相対姿勢を推定
 * @param[in] view1               カメラ1の内部パラメータと特徴点 (最低でも13点)
 * @param[in] view2               view1 に対応するカメラ2の内部パラメータと特徴点
 * @param[in] max_iteration_count 最大反復回数
 * @return カメラ1からカメラ2への相対姿勢
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 * @exception 相対姿勢の推定に失敗した場合に std::runtime_error が発生
 */
Pose3 getRelativePose(const View &view1, const View &view2, const size_t max_iteration_count);

/**
 * @brief カメラの姿勢を推定
 * @param[in] view      カメラ内部パラメータと特徴点のスクリーン座標
 * @param[in] points_3d view.points に対応する特徴点の現実座標 (3 x N)
 * @return カメラの姿勢
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 * @exception 姿勢の推定に失敗した場合に std::runtime_error が発生
 */
Pose3 getPose(const View &view, const Mat &points_3d);

/**
 * @brief 特徴点の三次元座標を三角測量
 * @param[in] view1 カメラ1 (内部パラメータ+姿勢) と特徴点のスクリーン座標
 * @param[in] view2 view1 に対応するカメラ2と特徴点のスクリーン座標
 * @return 特徴点の三次元座標 (3 x N)
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 */
Mat triangulation(const PosedView &view1, const PosedView &view2);

/**
 * @brief 初期姿勢から再投影誤差を最小化してカメラ姿勢を精密化 (LM)
 * @param[in] view       カメラ内部パラメータと特徴点のスクリーン座標
 * @param[in] points_3d  view.points に対応する特徴点の現実座標 (3 x N)
 * @param[in] initial    初期姿勢
 * @param[in] max_iterations 最大反復回数
 * @param[in] dof_mask   カメラフレームの自由度ビットマスク (上位から tx,ty,tz,wx,wy,wz)
 *                        0b111111 = 全6自由度, 0b110000 = tx,ty のみ, 0b111001 = tx,ty,tz,wz
 * @return 精密化された姿勢
 * @exception 引数の行列サイズが不正だった場合に std::invalid_argument が発生
 */
Pose3 refinePose(const View &view, const Mat &points_3d, const Pose3 &initial, size_t max_iterations = 20, uint32_t dof_mask = 0b111111);

/**
 * @brief バンドル調整
 * @param[in] scene バンドル調整を行うシーン
 * @return バンドル調整後のシーン
 * @exception バンドル調整に失敗した場合に std::runtime_error が発生
 */
Scene bundleAdjustment(const Scene &scene);

#endif  // ifndef WASM_MVG
