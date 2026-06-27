#ifndef WASM_MVG_BINDINGS
#define WASM_MVG_BINDINGS

#include "wasmMVG.h"

#ifdef __EMSCRIPTEN__

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

using Val = emscripten::val;
Val error(const std::string &description);
Val ok(const Val &value);
Mat valToMat(const Val &val);
Val matToVal(const Mat &mat);
Mat valToPoints(const Val &val, size_t rows);
Val pointsToVal(const Mat &mat);
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
Val refinePoseJs(const Val &intrinsic, const Val &points_2d, const Val &points_3d, const Val &initial_pose, size_t max_iterations, size_t dof_mask);
Val triangulationJs(const Val &cam1, const Val &cam1_points, const Val &cam2, const Val &cam2_points);
Val bundleAdjustmentJs(const Val &scene);

#endif  // ifdef __EMSCRIPTEN__
#endif  // ifndef WASM_MVG_BINDINGS
