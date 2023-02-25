#include "wasmMVG.h"
#include "utils.h"
#include <gtest/gtest.h>

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
TEST(sceneToSfM_Data, normal) {
	std::mt19937 rnd(123);
	const Scene scene = randomScene(rnd);
	const SfM_Data sfm_data = sceneToSfM_Data(scene);
	/*
	 * TODO: scene と sfm_data が同じ構造のシーンであることを確認
	 */
}
TEST(SfM_DataToScene, normal) {
	/*
	 * TODO: SfM_DataToScene のテストを作成
	 */
}
TEST(getRelativePose, normal) {
	/*
	 * TODO: getRelativePose のテストを作成
	 */
}
TEST(getPose, normal) {
	/*
	 * TODO: getPose のテストを作成
	 */
}
TEST(triangulation, normal) {
	/*
	 * TODO: triangulation のテストを作成
	 */
}
TEST(bundleAdjustment, normal) {
	/*
	 * TODO: bundleAdjustment のテストを作成
	 */
}
