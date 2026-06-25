export type Error = {
  result: "error",
  description: string,
};
export type Ok<T> = {
  result: "ok",
  value: T,
};
export type Result<T> = Ok<T> | Error;
export type Vec2 = [number, number];
export type Vec3 = [number, number, number];
export type Mat3 = [Vec3, Vec3, Vec3];
export type Pose = {
  rotation: Mat3,
  translation: Vec3,
};
export interface Intrinsic {
  width: number;
  height: number;
  type: "PINHOLE_CAMERA_RADIAL3";
}
export class PinholeIntrinsicRadialK3 implements Intrinsic {
  width: number;
  height: number;
  type: "PINHOLE_CAMERA_RADIAL3";
  focal: number;
  ppx: number;
  ppy: number;
  k1: number;
  k2: number;
  k3: number;
}
export type Camera = {
  intrinsic: Intrinsic,
  pose: Pose,
};
export type Scene = {
  cameras: {[key: number]: Camera},
  points: {[key: number]: {
    point: Vec3,
    cameras: {[key: number]: Vec2},
  }},
};
export type Module = {
  hello: (name: string) => string,
  // 点群はフラットな Float64Array (列優先 = 点ごとに連続) で受け渡す。
  // 2D 点群は [x0,y0,x1,y1,...]、3D 点群は [x0,y0,z0,x1,y1,z1,...] の並び。
  getRelativePose: (
    cam1_intrinsic: Intrinsic, cam1_points: Float64Array,
    cam2_intrinsic: Intrinsic, cam2_points: Float64Array,
    max_iteration_count: number
  ) => Result<Pose>,
  getPose: (intrinsic: Intrinsic, points_2d: Float64Array, points_3d: Float64Array) => Result<Pose>,
  refinePose: (intrinsic: Intrinsic, points_2d: Float64Array, points_3d: Float64Array, initial_pose: Pose, max_iterations: number, dof_mask: number) => Result<Pose>,
  triangulation: (cam1: Camera, cam1_points: Float64Array, cam2: Camera, cam2_points: Float64Array) => Result<Float64Array>,
  bundleAdjustment: (scene: Scene) => Result<Scene>,
};
// emscripten のファクトリへ渡すモジュール引数 (任意)。
// noInitialRun などの既知オプションに加え、その他の emscripten 設定も受け付ける。
export type ModuleArg = {
  noInitialRun?: boolean,
  [key: string]: unknown,
};
declare function wasmMVG(moduleArg?: ModuleArg): Promise<Module>;
export default wasmMVG;
