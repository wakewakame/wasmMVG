import renderer from "./renderer.js";
import vector from "./vector.js";

const main = () => {
	// 配置する点群の座標
	const points = [
		[-1, -1, -1],
		[+1, -1, -1],
		[+1, +1, -1],
		[-1, +1, -1],
		[-1, -1, +1],
		[+1, -1, +1],
		[+1, +1, +1],
		[-1, +1, +1],
	];
	for(let i = 0; i < 8; i++) {
		const x = Math.random() * 2 - 1;
		const y = Math.random() * 2 - 1;
		const z = Math.random() * 2 - 1;
		points.push([x, y, z]);
	}

	// canvasの取得
	const cam1_canvas = document.getElementById("cam1");  // カメラ1
	const cam2_canvas = document.getElementById("cam2");  // カメラ2
	const preview_canvas = document.getElementById("preview");  // 全体像把握用のプレビュー

	// シーンの作成
	const scene = new renderer.Scene();

	// カメラと点群のモデルを生成し、シーンに追加
	const cam1_cam = new renderer.CameraModelWithMouse(cam1_canvas);
	const cam2_cam = new renderer.CameraModelWithMouse(cam2_canvas);
	const preview_cam = new renderer.CameraModelWithMouse(preview_canvas, "#00000000");
	const axis_model = new renderer.AxisModel();
	const points_model = new renderer.PointsModel("#77A9B0", points);
	scene.addModel(cam1_cam);
	scene.addModel(cam2_cam);
	scene.addModel(preview_cam);
	scene.addModel(axis_model);
	scene.addModel(points_model);

	// 8点アルゴリズムによって求めたカメラと点群の位置をシーンに追加
	const cam2_cam_reconstructed = new renderer.VirtualCameraModel(0, 0);
	const points_model_reconstructed = new renderer.PointsModel("#CCA990");
	scene.addModel(cam2_cam_reconstructed);
	scene.addModel(points_model_reconstructed);

	const calc = async () => {
		// 回転行列と平行移動ベクトル、点群の三次元座標を計算
		const [w1, h1] = [cam1_canvas.width, cam1_canvas.height];
		const [w2, h2] = [cam2_canvas.width, cam2_canvas.height];
		const cam1 = {
			type: "PINHOLE_CAMERA_RADIAL3",
			width: w1, height: h1, focal: (w1 + h1) / 2,
			ppx: w1 / 2, ppy: h1 / 2, k1: 0, k2: 0, k3: 0
		};
		const cam2 = {
			type: "PINHOLE_CAMERA_RADIAL3",
			width: w2, height: h2, focal: (w2 + h2) / 2,
			ppx: w2 / 2, ppy: h2 / 2, k1: 0, k2: 0, k3: 0
		};
		const points1 = cam1_cam.worldToScreen(points).map(point => [point.x, point.y]);
		const points2 = cam2_cam.worldToScreen(points).map(point => [point.x, point.y]);
		const data = Module.getRelativePose(cam1, points1, cam2, points2, 4096);
		if (data === null) { return; }

		const data2 = Module.triangulation(
			{
				image_path: "", intrinsic: cam1,
				pose: { rotation: [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]], translation: [0.0, 0.0, 0.0] }
			},
			points1,
			{
				image_path: "", intrinsic: cam2,
				pose: { rotation: data["rotation"], translation: data["translation"] }
			},
			points2
		);
		if (data2 === null) { return; }
		data["points"] = data2;
		const data3 = Module.getPose(cam2, points2, data.points);
		if (data3 === null) { return; }
		data["rotation"] = data3["rotation"];
		data["translation"] = data3["translation"];
		const data4_input = {
			"cameras": {
				"1": {
					image_path: "", intrinsic: cam1,
					pose: { rotation: [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]], translation: [0.0, 0.0, 0.0] }
				},
				"2": {
					image_path: "", intrinsic: cam2,
					pose: { rotation: data["rotation"], translation: data["translation"] }
				},
			},
			"points": Object.fromEntries(data.points.map((point, index) => ([index, { point: point, cameras: { "1": points1[index], "2": points2[index] } }])))
		};
		const data4 = Module.bundleAdjustment(data4_input);
		data["points"] = Object.values(data4["points"]).map(point => point.point);
		data["rotation"] = data4["cameras"]["2"]["pose"]["rotation"];
		data["translation"] = data4["cameras"]["2"]["pose"]["translation"];

		const Rt_ = DOMMatrix.fromFloat64Array(new Float64Array([
			data["rotation"][0][0], data["rotation"][0][1], data["rotation"][0][2], 0,
			data["rotation"][1][0], data["rotation"][1][1], data["rotation"][1][2], 0,
			data["rotation"][2][0], data["rotation"][2][1], data["rotation"][2][2], 0,
			data["translation"][0], data["translation"][1], data["translation"][2], 1
		]));
		let points_ = data["points"];

		// 現実の回転行列と平行移動ベクトルを計算
		const cam1_view = vector.normalizeScale(cam1_cam.view);
		const cam2_view = vector.normalizeScale(cam2_cam.view);
		const Rt = cam2_view.multiply(cam1_view.inverse());

		// Rt_とRtの平行移動成分の長さから、Rt_とpoints_のスケールをRtとpointsに合わせる
		const Rt_length_ = Math.sqrt(Rt_.m41 * Rt_.m41 + Rt_.m42 * Rt_.m42 + Rt_.m43 * Rt_.m43);
		const Rt_length = Math.sqrt(Rt.m41 * Rt.m41 + Rt.m42 * Rt.m42 + Rt.m43 * Rt.m43);
		const scale = Rt_length / Rt_length_;
		Rt_.m41 *= scale; Rt_.m42 *= scale; Rt_.m43 *= scale;
		points_ = points_.map(([x, y, z]) => [x * scale, y * scale, z * scale]);

		// points_をcam1座標系から世界座標系へ変形
		points_ = points_.map(([x, y, z]) => {
			let point = new DOMPoint(x, y, z, 1);
			point = cam1_view.inverse().transformPoint(point);
			return [point.x, point.y, point.z, 1];
		});

		// プレビューの更新
		cam2_cam_reconstructed.width      = cam2_cam.width;
		cam2_cam_reconstructed.height     = cam2_cam.height;
		cam2_cam_reconstructed.view       = Rt_.multiply(cam1_view);
		cam2_cam_reconstructed.projection = cam2_cam.projection;
		points_model_reconstructed.points = points_;
		scene.update();
	};
	cam1_canvas.addEventListener("pointerup", async () => { calc(); });
	cam2_canvas.addEventListener("pointerup", async () => { calc(); });
};

document.addEventListener("DOMContentLoaded", () => {
	window.Module = {
		// wasm の読み込みが完了したときのコールバック
		onRuntimeInitialized: () => { main(); },
		// main 関数の自動実行を無効化
		noInitialRun: true
	};

	// wasm 読み込み
	const scriptElem = document.createElement("script");
	scriptElem.src = "../build/wasm/RelWithDebInfo/main.js";
	document.body.appendChild(scriptElem);
});
