#ifndef WASM_MVG_TEST_UTILS
#define WASM_MVG_TEST_UTILS

#include <random>
#include "wasmMVG.h"

double randomInt(std::mt19937 &rnd, const int64_t min, const int64_t max) {
	std::uniform_int_distribution<int64_t> dis(min, max);
	return dis(rnd);
}
double randomDouble(std::mt19937 &rnd, const double min, const double max) {
	std::uniform_real_distribution<double> dis(min, max);
	return dis(rnd);
}
Mat randomMat(std::mt19937 &rnd, const size_t rows, const size_t cols) {
	return Mat::NullaryExpr(rows, cols, [&]() { return randomDouble(rnd, -1.0, 1.0); });
}
Mat randomVec(std::mt19937 &rnd, const size_t rows) {
	return Vec::NullaryExpr(rows, [&]() { return randomDouble(rnd, -1.0, 1.0); });
}
Mat randomRotate(std::mt19937 &rnd) {
	const Vec3 x = Vec3(randomVec(rnd, 3)).normalized();
	const Vec3 y = Vec3(randomVec(rnd, 3)).cross(x).normalized();
	const Vec3 z = x.cross(y);
	Mat mat { 3, 3 }; mat.col(0) = x; mat.col(1) = y; mat.col(2) = z;
	return mat;
}
Intrinsic randomIntrinsic(std::mt19937 &rnd, const size_t type = 0) {
	Intrinsic intrinsic;
	const int width  = randomInt(rnd, 100, 1000);
	const int height = randomInt(rnd, 100, 1000);
	if (type == 0) {
		intrinsic.reset(new openMVG::cameras::Pinhole_Intrinsic_Radial_K3 {
			width, height,
			randomDouble(rnd, 100, 1000),  // focal
			randomDouble(rnd, 0, width),   // ppx
			randomDouble(rnd, 0, height),  // ppy
			randomDouble(rnd, 0.0, 10.0),  // k1
			randomDouble(rnd, 0.0, 10.0),  // k2
			randomDouble(rnd, 0.0, 10.0)   // k3
		});
	}
	return intrinsic;
}
Pose3 randomPose(std::mt19937 &rnd) {
	return Pose3{ randomRotate(rnd), randomVec(rnd, 3) };
}
Camera randomCamera(std::mt19937 &rnd) {
	return Camera{ randomIntrinsic(rnd), randomPose(rnd) };
}
Point randomPoint(std::mt19937 &rnd, const std::map<size_t, Camera> &cameras) {
	std::map<size_t, Vec2> camera_points;
	for(const std::pair<const size_t, Camera> &camera : cameras) {
		if (randomInt(rnd, 0, 3) == 0) {
			camera_points[camera.first] = randomVec(rnd, 2);
		}
	}
	return Point{ randomVec(rnd, 3), camera_points };
}
Scene randomScene(std::mt19937 &rnd, const size_t cameras = 3, const size_t points = 300) {
	Scene scene;
	for(size_t i = 0; i < cameras; i++) {
		scene.cameras[rnd()] = randomCamera(rnd);
	}
	for(size_t i = 0; i < points; i++) {
		scene.points[rnd()] = randomPoint(rnd, scene.cameras);
	}
	return scene;
}

// 歪みのない PINHOLE_CAMERA_RADIAL3 カメラの内部パラメータを生成
Intrinsic pinholeIntrinsic(const int width, const int height, const double focal) {
	Intrinsic intrinsic;
	intrinsic.reset(new openMVG::cameras::Pinhole_Intrinsic_Radial_K3 {
		width, height, focal, width / 2.0, height / 2.0, 0.0, 0.0, 0.0
	});
	return intrinsic;
}
// ワールド座標の点をカメラのスクリーン座標へ投影
Vec2 projectPoint(const Camera &camera, const Vec3 &point) {
	return camera.intrinsic->project(camera.pose(point));
}
// 複数のワールド座標 (3 x N) をスクリーン座標 (2 x N) へまとめて投影
Mat projectPoints(const Camera &camera, const Mat &points_3d) {
	Mat points_2d{ 2, points_3d.cols() };
	for (size_t col = 0; col < (size_t)points_3d.cols(); col++) {
		points_2d.col(col) = projectPoint(camera, Vec3(points_3d.col(col)));
	}
	return points_2d;
}
// テスト用の合成ステレオデータ
struct StereoTestData {
	Camera cam1;
	Camera cam2;
	Mat points_3d;   // 3 x N のワールド座標
	Mat points1_2d;  // 2 x N の cam1 スクリーン座標
	Mat points2_2d;  // 2 x N の cam2 スクリーン座標
};
// 原点付近の点群を 2 台のカメラから観測した、歪み・ノイズのない整合データを生成する。
// 2 台のカメラは点群の手前 (z = -5) に x 方向の基線を空けて配置し、いずれも +z 方向を向く。
StereoTestData makeStereoTestData(std::mt19937 &rnd, const size_t num_points = 64) {
	StereoTestData data;
	data.cam1 = Camera{ pinholeIntrinsic(640, 480, 500.0), Pose3{ openMVG::Mat3::Identity(), Vec3(0.0, 0.0, -5.0) } };
	data.cam2 = Camera{ pinholeIntrinsic(640, 480, 500.0), Pose3{ openMVG::Mat3::Identity(), Vec3(2.0, 0.0, -5.0) } };
	data.points_3d = Mat{ 3, num_points };
	for (size_t col = 0; col < num_points; col++) {
		data.points_3d.col(col) = Vec3(
			randomDouble(rnd, -1.0, 1.0),
			randomDouble(rnd, -1.0, 1.0),
			randomDouble(rnd, -1.0, 1.0)
		);
	}
	data.points1_2d = projectPoints(data.cam1, data.points_3d);
	data.points2_2d = projectPoints(data.cam2, data.points_3d);
	return data;
}

#endif
