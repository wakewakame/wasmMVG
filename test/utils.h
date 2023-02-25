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

#endif
