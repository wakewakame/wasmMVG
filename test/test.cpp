#include "wasmMVG.h"
#include "utils.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>

TEST(columnMap, normal) {
	std::mt19937 rnd(123);
	const Mat mat = randomMat(rnd, 3, 4);
	const Vec add = randomVec(rnd, 3);
	Mat expect{ mat.rows(), mat.cols() };
	for(size_t col = 0; col < mat.cols(); col++) {
		expect.col(col) = mat.col(col) + add;
	}
	const Mat actual = columnMap(mat, [add](const Vec &vec) { return vec + add; });
	EXPECT_EQ(expect, actual);
}

// Scene -> SfM_Data -> Scene と往復しても内容が保存されることを確認
TEST(sceneToSfM_Data, roundTrip) {
	std::mt19937 rnd(123);
	const Scene scene = randomScene(rnd);
	const Scene restored = SfM_DataToScene(sceneToSfM_Data(scene));

	EXPECT_EQ(scene.cameras.size(), restored.cameras.size());
	EXPECT_EQ(scene.points.size(), restored.points.size());

	for (const auto &[key, camera] : scene.cameras) {
		ASSERT_TRUE(restored.cameras.count(key));
		const Camera &r = restored.cameras.at(key);
		EXPECT_TRUE(camera.pose.rotation().isApprox(r.pose.rotation()));
		EXPECT_TRUE(camera.pose.center().isApprox(r.pose.center()));
		EXPECT_EQ(camera.intrinsic->getType(), r.intrinsic->getType());
		EXPECT_EQ(camera.intrinsic->getParams(), r.intrinsic->getParams());
	}

	for (const auto &[key, point] : scene.points) {
		ASSERT_TRUE(restored.points.count(key));
		const Point &r = restored.points.at(key);
		EXPECT_TRUE(point.point.isApprox(r.point));
		EXPECT_EQ(point.cameras.size(), r.cameras.size());
		for (const auto &[cam_key, obs] : point.cameras) {
			ASSERT_TRUE(r.cameras.count(cam_key));
			EXPECT_TRUE(obs.isApprox(r.cameras.at(cam_key)));
		}
	}
}

// 既知の姿勢を持つ 2 台のカメラからの投影を三角測量し、元の三次元座標を復元できることを確認
TEST(triangulation, recoversPoints) {
	std::mt19937 rnd(123);
	const StereoTestData data = makeStereoTestData(rnd);
	const Mat recovered = triangulation(
		PosedView{ data.cam1, data.points1_2d },
		PosedView{ data.cam2, data.points2_2d }
	);
	ASSERT_EQ(recovered.cols(), data.points_3d.cols());
	for (size_t col = 0; col < (size_t)data.points_3d.cols(); col++) {
		EXPECT_TRUE(recovered.col(col).isApprox(data.points_3d.col(col), 1e-4))
			<< "col " << col
			<< " expected (" << data.points_3d.col(col).transpose() << ")"
			<< " got (" << recovered.col(col).transpose() << ")";
	}
}

// 既知の三次元座標とその投影からカメラ姿勢を復元できることを確認
TEST(getPose, recoversPose) {
	std::mt19937 rnd(123);
	const StereoTestData data = makeStereoTestData(rnd);
	const Pose3 pose = getPose(View{ data.cam2.intrinsic, data.points2_2d }, data.points_3d);
	EXPECT_TRUE(pose.rotation().isApprox(data.cam2.pose.rotation(), 1e-4));
	EXPECT_TRUE(pose.center().isApprox(data.cam2.pose.center(), 1e-4));
}

// 初期姿勢に摂動を加えた状態から refinePose で真の姿勢を復元できることを確認
TEST(refinePose, recoversFromPerturbedPose) {
	const auto intrinsic = pinholeIntrinsic(640, 480, 500.0);
	const double ay = M_PI / 6.0, ax = M_PI / 9.0;
	openMVG::Mat3 Ry, Rx;
	Ry << std::cos(ay), 0, std::sin(ay),
	      0,            1, 0,
	     -std::sin(ay), 0, std::cos(ay);
	Rx << 1, 0,             0,
	      0, std::cos(ax), -std::sin(ax),
	      0, std::sin(ax),  std::cos(ax);
	const openMVG::Mat3 R = Ry * Rx;
	const Pose3 gt_pose(R, Vec3(0.0, 0.0, -5.0));
	const Camera cam{ intrinsic, gt_pose };

	Mat points_3d(3, 8);
	points_3d.col(0) = Vec3(-1, -1, -1);
	points_3d.col(1) = Vec3( 1, -1, -1);
	points_3d.col(2) = Vec3( 1,  1, -1);
	points_3d.col(3) = Vec3(-1,  1, -1);
	points_3d.col(4) = Vec3(-1, -1,  1);
	points_3d.col(5) = Vec3( 1, -1,  1);
	points_3d.col(6) = Vec3( 1,  1,  1);
	points_3d.col(7) = Vec3(-1,  1,  1);

	const Mat points_2d = projectPoints(cam, points_3d);

	// 真の姿勢に摂動を加えた初期値
	const Pose3 perturbed(R * Eigen::AngleAxisd(0.1, Vec3::UnitY()).toRotationMatrix(),
	                      Vec3(0.2, -0.1, -4.8));

	const Pose3 recovered = refinePose(View{ intrinsic, points_2d }, points_3d, perturbed, 50);
	EXPECT_TRUE(recovered.rotation().isApprox(gt_pose.rotation(), 1e-6))
		<< "rotation expected:\n" << gt_pose.rotation()
		<< "\ngot:\n" << recovered.rotation();
	EXPECT_TRUE(recovered.center().isApprox(gt_pose.center(), 1e-6))
		<< "center expected: " << gt_pose.center().transpose()
		<< " got: " << recovered.center().transpose();
}

// refinePose で少数 (4 点) でも全点が影響して姿勢が復元されることを確認
TEST(refinePose, worksWithFourPoints) {
	const auto intrinsic = pinholeIntrinsic(640, 480, 500.0);
	const Pose3 gt_pose(openMVG::Mat3::Identity(), Vec3(0.0, 0.0, -5.0));
	const Camera cam{ intrinsic, gt_pose };

	Mat points_3d(3, 4);
	points_3d.col(0) = Vec3(-1, -1, 0);
	points_3d.col(1) = Vec3( 1, -1, 0);
	points_3d.col(2) = Vec3( 1,  1, 0);
	points_3d.col(3) = Vec3(-1,  1, 0);

	const Mat points_2d = projectPoints(cam, points_3d);

	const Pose3 perturbed(openMVG::Mat3::Identity(), Vec3(0.1, -0.1, -4.9));

	const Pose3 recovered = refinePose(View{ intrinsic, points_2d }, points_3d, perturbed, 50);
	EXPECT_TRUE(recovered.rotation().isApprox(gt_pose.rotation(), 1e-4));
	EXPECT_TRUE(recovered.center().isApprox(gt_pose.center(), 1e-4))
		<< "center expected: " << gt_pose.center().transpose()
		<< " got: " << recovered.center().transpose();
}

// 1 点 + tx,ty のみで平行移動を追従できることを確認
TEST(refinePose, onePointTranslation) {
	const auto intrinsic = pinholeIntrinsic(640, 480, 500.0);
	const Pose3 gt_pose(openMVG::Mat3::Identity(), Vec3(0.3, -0.2, -5.0));
	const Camera cam_gt{ intrinsic, gt_pose };

	Mat points_3d(3, 1);
	points_3d.col(0) = Vec3(0, 0, 0);
	const Mat points_2d = projectPoints(cam_gt, points_3d);

	// 初期姿勢はカメラ中心が少しずれている
	const Pose3 initial(openMVG::Mat3::Identity(), Vec3(0.0, 0.0, -5.0));

	// dof_mask = 0b110000 (tx, ty のみ)
	const Pose3 recovered = refinePose(View{ intrinsic, points_2d }, points_3d, initial, 50, 0b110000);

	// tx, ty は目標に近づくが、tz, 回転は初期値のまま
	const Vec3 t_recovered = recovered.translation();
	const Vec3 t_gt = gt_pose.translation();
	EXPECT_NEAR(t_recovered(0), t_gt(0), 1e-6);
	EXPECT_NEAR(t_recovered(1), t_gt(1), 1e-6);
	EXPECT_NEAR(t_recovered(2), initial.translation()(2), 1e-10);
	EXPECT_TRUE(recovered.rotation().isApprox(openMVG::Mat3::Identity(), 1e-10));
}

// 2 点 + wz,tx,ty,tz でスケールと画面内回転を追従できることを確認
TEST(refinePose, twoPointsScaleAndRotation) {
	const auto intrinsic = pinholeIntrinsic(640, 480, 500.0);
	// 画面内回転 (rz=15°) + 奥行き変更でターゲットを生成
	const double rz = M_PI / 12.0;
	openMVG::Mat3 Rz;
	Rz << std::cos(rz), -std::sin(rz), 0,
	      std::sin(rz),  std::cos(rz), 0,
	      0,             0,            1;
	const Pose3 gt_pose(Rz, Vec3(0.1, -0.1, -4.0));
	const Camera cam_gt{ intrinsic, gt_pose };

	Mat points_3d(3, 2);
	points_3d.col(0) = Vec3(-1, 0, 0);
	points_3d.col(1) = Vec3( 1, 0, 0);
	const Mat points_2d = projectPoints(cam_gt, points_3d);

	const Pose3 initial(openMVG::Mat3::Identity(), Vec3(0.0, 0.0, -5.0));

	// dof_mask = 0b111001 (tx, ty, tz, wz)
	const Pose3 recovered = refinePose(View{ intrinsic, points_2d }, points_3d, initial, 100, 0b111001);

	// 再投影誤差が十分小さいことを確認
	const Camera cam_recovered{ intrinsic, recovered };
	for (int i = 0; i < 2; i++) {
		const Vec2 reproj = projectPoint(cam_recovered, Vec3(points_3d.col(i)));
		EXPECT_TRUE(reproj.isApprox(Vec2(points_2d.col(i)), 1e-3))
			<< "point " << i << " expected: " << points_2d.col(i).transpose()
			<< " got: " << reproj.transpose();
	}
}

// 2 視点の対応点から相対姿勢を復元できることを確認 (並進はスケール不定)
TEST(getRelativePose, recoversRelativePose) {
	std::mt19937 rnd(123);
	const StereoTestData data = makeStereoTestData(rnd);
	const Pose3 pose = getRelativePose(
		View{ data.cam1.intrinsic, data.points1_2d },
		View{ data.cam2.intrinsic, data.points2_2d },
		4096
	);
	// 真の相対姿勢: 回転 = R2 * R1^T, 並進方向 = R2 * (c1 - c2)
	const openMVG::Mat3 R_rel = data.cam2.pose.rotation() * data.cam1.pose.rotation().transpose();
	const Vec3 t_rel = data.cam2.pose.rotation() * (data.cam1.pose.center() - data.cam2.pose.center());
	EXPECT_TRUE(pose.rotation().isApprox(R_rel, 1e-3));
	// 並進はスケール不定なので方向のみ比較 (符号も許容)
	const Vec3 t_est = pose.translation().normalized();
	const Vec3 t_gt = t_rel.normalized();
	EXPECT_NEAR(std::abs(t_est.dot(t_gt)), 1.0, 1e-3);
}

// 整合したシーンをバンドル調整しても、再投影誤差が小さいまま保たれることを確認
TEST(bundleAdjustment, keepsConsistentScene) {
	std::mt19937 rnd(123);
	const StereoTestData data = makeStereoTestData(rnd);

	Scene scene;
	scene.cameras[0] = data.cam1;
	scene.cameras[1] = data.cam2;
	for (size_t col = 0; col < (size_t)data.points_3d.cols(); col++) {
		Point point;
		point.point = data.points_3d.col(col);
		point.cameras[0] = data.points1_2d.col(col);
		point.cameras[1] = data.points2_2d.col(col);
		scene.points[col] = point;
	}

	const Scene adjusted = bundleAdjustment(scene);
	ASSERT_EQ(adjusted.cameras.size(), scene.cameras.size());
	ASSERT_EQ(adjusted.points.size(), scene.points.size());

	// バンドル調整にはゲージ自由度があるため、ゲージ不変な再投影誤差で評価する
	double max_error = 0.0;
	for (const auto &point_entry : adjusted.points) {
		const Point &point = point_entry.second;
		for (const auto &[cam_key, obs] : point.cameras) {
			const Vec2 reproj = projectPoint(adjusted.cameras.at(cam_key), point.point);
			max_error = std::max(max_error, (reproj - obs).norm());
		}
	}
	EXPECT_LT(max_error, 1e-2);
}
