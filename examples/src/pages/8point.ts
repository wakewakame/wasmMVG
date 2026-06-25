import renderer from '../utils/renderer';
import vector from '../utils/vector';
import wasmMVG from 'wasmMVG';
import type { Module, Camera, Scene, Vec2, Vec3 } from 'wasmMVG';

// 点群を wasm へ渡すためのフラットな Float64Array (列優先 = 点ごとに連続) に変換する。
// 例: [[x0,y0],[x1,y1]] -> Float64Array[x0,y0,x1,y1]
const flatPoints2 = (points: number[][]): Float64Array => {
  const out = new Float64Array(points.length * 2);
  points.forEach((p, i) => { out[i * 2] = p[0]; out[i * 2 + 1] = p[1]; });
  return out;
};
const flatPoints3 = (points: number[][]): Float64Array => {
  const out = new Float64Array(points.length * 3);
  points.forEach((p, i) => { out[i * 3] = p[0]; out[i * 3 + 1] = p[1]; out[i * 3 + 2] = p[2]; });
  return out;
};
// wasm から返ったフラットな Float64Array を [[x,y,z],...] へ戻す。
const unflatPoints3 = (flat: Float64Array | number[]): number[][] => {
  const out: number[][] = [];
  for (let i = 0; i < flat.length; i += 3) { out.push([flat[i], flat[i + 1], flat[i + 2]]); }
  return out;
};

const main = async (Module: Module): Promise<void> => {
  // 配置する点群の座標
  const plyAscii = await fetch("../example_images/model.ply").then(res => res.text());
  const plyPointLength = Number(plyAscii.match(/element vertex [0-9]+/)![0].match(/[0-9]+/)![0]);
  const plyFaceLength = Number(plyAscii.match(/element face [0-9]+/)![0].match(/[0-9]+/)![0]);
  const plyEndHeaderLine = plyAscii.split("\n").findIndex(line => line === "end_header");
  let points =
    plyAscii.split("\n").slice(plyEndHeaderLine + 1, plyEndHeaderLine + plyPointLength + 1).
    map(point => point.split(" ").map(num => Number(num) * 3.0)).map(([x, y, z]) => [x, -y, -z]);
  const points_center = points
    .reduce((acc, p) => acc.map((x, i) => (x + p[i])), [0, 0, 0])
    .map(p => (p / points.length));
  points = points.map(p => (p.map((x, i) => (x - points_center[i]))));
  const faces =
    plyAscii.split("\n").slice(plyEndHeaderLine + plyPointLength + 1, plyEndHeaderLine + plyPointLength + plyFaceLength + 1).
    map(point => point.split(" ").map(num => Number(num)).slice(1));

  // canvasの取得
  const preview_canvas = document.getElementById("preview") as HTMLCanvasElement;  // 全体像把握用のプレビュー

  // シーンの作成
  const scene = new renderer.Scene();

  // カメラと点群のモデルを生成し、シーンに追加
  const num_camera = 5;
  const px_rand = 4.0;
  const focal_rand = 0.0;
  const circle_cam = [...new Array(num_camera)].map((_, index) => {
    const rotate = vector.rotate(new DOMPoint(0, 1, 0, 1), Math.PI * 2 * index / num_camera);
    const p = index / (num_camera - 1);
    const color = [0xFC * (1 - p) + 0xF9 * p, 0xE2 * (1 - p) + 0x4A * p, 0x2A * (1 - p) + 0x29 * p];
    const cam = new renderer.VirtualCameraModel(640, 480, `#${color.map(c => ("00" + Math.floor(c).toString(16)).slice(-2)).join("")}`);
    cam.view.m43 -= 10;
    cam.view.preMultiplySelf(rotate);
    cam.view.m43 += 10;
    const cam_ = new renderer.VirtualCameraModel(640, 480, "#FF3090");
    cam_.view.m43 -= 10;
    cam_.view.preMultiplySelf(rotate);
    cam_.view.m43 += 10;
    scene.addModel(cam);
    scene.addModel(cam_);
    return [cam, cam_];
  });
  const preview_cam = new renderer.CameraModelWithMouse(preview_canvas, "#00000000");
  const axis_model = new renderer.AxisModel();
  const lines_model = new renderer.LinesModel("#77A9B0", points, faces);
  const points_reconstruct_model = new renderer.LinesModel("#FF3090", points, faces);
  scene.addModel(preview_cam);
  scene.addModel(axis_model);
  scene.addModel(lines_model);
  scene.addModel(points_reconstruct_model);

  let ba_scene: Scene = { cameras: {}, points: {} };
  for(let i = 0; i < circle_cam.length - 1; i++) {
    const cam1 = circle_cam[i][1];
    const cam2 = circle_cam[i + 1][0];
    const [w1, h1] = [cam1.width, cam1.height];
    const [w2, h2] = [cam2.width, cam2.height];
    const camera1 = {
      type: "PINHOLE_CAMERA_RADIAL3" as const,
      width: w1, height: h1, focal: (w1 + h1) / 2 + focal_rand * (Math.random() * 2.0 - 1.0),
      ppx: w1 / 2, ppy: h1 / 2, k1: 0, k2: 0, k3: 0
    };
    const camera2 = {
      type: "PINHOLE_CAMERA_RADIAL3" as const,
      width: w2, height: h2, focal: (w2 + h2) / 2 + focal_rand * (Math.random() * 2.0 - 1.0),
      ppx: w2 / 2, ppy: h2 / 2, k1: 0, k2: 0, k3: 0
    };
    const points1: Vec2[] = cam1.worldToScreen(points).map(point => [
      point.x + px_rand * (Math.random() * 2.0 - 1.0),
      point.y + px_rand * (Math.random() * 2.0 - 1.0)
    ]);
    const points2: Vec2[] = cam2.worldToScreen(points).map(point => [
      point.x + px_rand * (Math.random() * 2.0 - 1.0),
      point.y + px_rand * (Math.random() * 2.0 - 1.0)
    ]);
    const poseResult = (i === 0) ?
      Module.getRelativePose(camera1, flatPoints2(points1), camera2, flatPoints2(points2), 4096) :
      Module.getPose(camera2, flatPoints2(points2), flatPoints3(points_reconstruct_model.points));
    if (poseResult.result === "error") { continue; }
    const pose = poseResult.value;
    let Rt_ = DOMMatrix.fromFloat64Array(new Float64Array([
      pose.rotation[0][0], pose.rotation[0][1], pose.rotation[0][2], 0,
      pose.rotation[1][0], pose.rotation[1][1], pose.rotation[1][2], 0,
      pose.rotation[2][0], pose.rotation[2][1], pose.rotation[2][2], 0,
      pose.translation[0], pose.translation[1], pose.translation[2], 1
    ]));

    if (i === 0) {
      // 現実の回転行列と平行移動ベクトルを計算
      const cam1_view = vector.normalizeScale(cam1.view);
      const cam2_view = vector.normalizeScale(cam2.view);
      const Rt = cam2_view.multiply(cam1_view.inverse());

      // Rt_とRtの平行移動成分の長さから、Rt_とpoints_のスケールをRtとpointsに合わせる
      const Rt_length_ = Math.sqrt(Rt_.m41 * Rt_.m41 + Rt_.m42 * Rt_.m42 + Rt_.m43 * Rt_.m43);
      const Rt_length = Math.sqrt(Rt.m41 * Rt.m41 + Rt.m42 * Rt.m42 + Rt.m43 * Rt.m43);
      const scale = Rt_length / Rt_length_;
      Rt_.m41 *= scale; Rt_.m42 *= scale; Rt_.m43 *= scale;
      const scaledTranslation: Vec3 = [
        pose.translation[0] * scale,
        pose.translation[1] * scale,
        pose.translation[2] * scale
      ];

      const triResult = Module.triangulation(
        {
          intrinsic: camera1,
          pose: { rotation: [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]], translation: [0.0, 0.0, 0.0] }
        },
        flatPoints2(points1),
        {
          intrinsic: camera2,
          pose: { rotation: pose.rotation, translation: scaledTranslation }
        },
        flatPoints2(points2)
      );
      if (triResult.result === "error") { continue; }
      const triangulatedPoints = unflatPoints3(triResult.value);

      points_reconstruct_model.points = triangulatedPoints.map(([x, y, z]) => {
        let point = new DOMPoint(x, y, z, 1);
        point = cam1_view.inverse().transformPoint(point);
        return [point.x, point.y, point.z, 1];
      });

      Rt_ = Rt_.multiply(cam1_view);

      ba_scene.cameras[i] = {
        intrinsic: camera1,
        pose: {
          rotation: [
            [cam1_view.m11, cam1_view.m12, cam1_view.m13],
            [cam1_view.m21, cam1_view.m22, cam1_view.m23],
            [cam1_view.m31, cam1_view.m32, cam1_view.m33]
          ],
          translation: [cam1_view.m41, cam1_view.m42, cam1_view.m43]
        }
      };
      ba_scene.points = {};
      points_reconstruct_model.points.forEach((point, index) => {
        ba_scene.points[index] = {
          point: point.slice(0, 3) as Vec3,
          cameras: {}
        };
        ba_scene.points[index].cameras[i] = points1[index];
      });
    }

    ba_scene.cameras[i + 1] = {
      intrinsic: camera2,
      pose: {
        rotation: [
          [Rt_.m11, Rt_.m12, Rt_.m13],
          [Rt_.m21, Rt_.m22, Rt_.m23],
          [Rt_.m31, Rt_.m32, Rt_.m33]
        ],
        translation: [Rt_.m41, Rt_.m42, Rt_.m43]
      }
    };
    points2.forEach((point, index) => {
      ba_scene.points[index].cameras[i + 1] = point;
    });

    // プレビューの更新
    circle_cam[i + 1][1].view = Rt_;
    //cam2.projection = cam2_cam.projection;

    if ((i === num_camera-1) || (i % 50 === 0)) {
      const ba_result = Module.bundleAdjustment(ba_scene);
      if (ba_result.result === "error") { continue; }
      ba_scene = ba_result.value;
      (Object.entries(ba_scene.cameras) as [string, Camera][]).forEach(([index, camera]) => {
        const { rotation, translation } = camera.pose;
        const ba_Rt = DOMMatrix.fromFloat64Array(new Float64Array([
          rotation[0][0], rotation[0][1], rotation[0][2], 0,
          rotation[1][0], rotation[1][1], rotation[1][2], 0,
          rotation[2][0], rotation[2][1], rotation[2][2], 0,
          translation[0], translation[1], translation[2], 1
        ]));
        circle_cam[Number(index)][1].view = ba_Rt;
      });
      (Object.entries(ba_scene.points) as [string, Scene["points"][number]][]).forEach(([index, point]) => {
        points_reconstruct_model.points[Number(index)] = [...point.point, 1];
      });
    }
  }

  // 重心、回転、スケールを撮影モデルと推定モデルで一致させる
  const model_center = points_reconstruct_model.points
    .reduce((acc, p) => acc.map((x, i) => (x + p[i])), [0, 0, 0])
    .map(p => (p / points_reconstruct_model.points.length));
  circle_cam.forEach(([, cam]) => {
    cam.view.m41 -= model_center[0];
    cam.view.m42 -= model_center[1];
    cam.view.m43 -= model_center[2];
  });
  points_reconstruct_model.points = points_reconstruct_model.points.map(p => {
    model_center.forEach((m, i) => { p[i] -= m; });
    return p;
  });
  const rotate_scale_avg = points.reduce<[number[], number, number]>((acc, p1, index) => {
    const p2 = points_reconstruct_model.points[index];
    const scale1 = Math.sqrt(p1[0] ** 2 + p1[1] ** 2 + p1[2] ** 2);
    const scale2 = Math.sqrt(p2[0] ** 2 + p2[1] ** 2 + p2[2] ** 2);
    const axis = [
      p1[1] * p2[2] - p1[2] * p2[1],
      p1[2] * p2[0] - p1[0] * p2[2],
      p1[0] * p2[1] - p1[1] * p2[0]
    ];
    const theta = Math.acos((p2[0] * p1[0] + p2[1] * p1[1] + p2[2] * p1[2]) / (scale1 * scale2));
    acc[0] = acc[0].map((ax, i) => (ax + axis[i]));
    acc[1] += isFinite(theta) ? theta : 0;
    acc[2] += (scale1 / scale2);
    return acc;
  }, [[0, 0, 0], 0, 0]);
  rotate_scale_avg[0] = rotate_scale_avg[0].map(ax => (ax / points.length));
  rotate_scale_avg[1] /= points.length;
  rotate_scale_avg[2] /= points.length;
  const rotate_avg2 = vector.rotate(new DOMPoint(...rotate_scale_avg[0]), rotate_scale_avg[1]);
  const rotate_avg3 = vector.rotate(new DOMPoint(...rotate_scale_avg[0]), -rotate_scale_avg[1]);
  points_reconstruct_model.points = points_reconstruct_model.points.map(p => {
    const point = rotate_avg2.transformPoint(new DOMPoint(p[0], p[1], p[2], p[3]));
    const scale = rotate_scale_avg[2];
    return [ point.x * scale, point.y * scale, point.z * scale, point.w ];
  });
  circle_cam.forEach(([, cam]) => {
    cam.view = cam.view.multiply(rotate_avg3);
    cam.view.m41 *= rotate_scale_avg[2];
    cam.view.m42 *= rotate_scale_avg[2];
    cam.view.m43 *= rotate_scale_avg[2];
  });
};

document.addEventListener("DOMContentLoaded", async () => {
  const Module = await wasmMVG({
    // main 関数の自動実行を無効化
    noInitialRun: true
  });
  main(Module);
});
