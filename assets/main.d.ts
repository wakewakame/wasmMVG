export type Error = {
  result: "error",
  description: string,
};
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
  getRelativePose: (
    cam1_intrinsic: Intrinsic, cam1_points: Camera,
    cam2_intrinsic: Intrinsic, cam2_points: Camera,
    max_iteration_count: number
  ) => Pose | Error,
  getPose: (intrinsic: Intrinsic, points_2d: Vec2[], points_3d: Vec3[]) => Pose | Error,
  triangulation: (cam1: Camera, cam1_points: Vec2[], cam2: Camera, cam2_points: Vec2[]) => Vec3[] | Error,
  bundleAdjustment: (scene: Scene) => Scene | Error,
};
declare function wasmMVG(): Promise<Module>;
export default wasmMVG;
