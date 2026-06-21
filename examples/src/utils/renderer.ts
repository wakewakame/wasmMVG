import vector from "./vector";

class Scene {
  models: Model[] = [];
  addModel<T extends Model>(model: T): T {
    model.scene = this;
    this.models.push(model);
    this.update();
    return model;
  }
  update(): void {
    this.models.forEach(model => { model.update(); });
  }
}

class Model {
  scene: Scene | null = null;
  update(): void {
  }
  draw(_cam: CameraModel): void {
  }
}

class VirtualCameraModel extends Model {
  width: number;
  height: number;
  view: DOMMatrix;
  projection: DOMMatrix;
  color: string;
  constructor(width: number, height: number, color: string = "#CCA990") {
    super();
    this.width = width;
    this.height = height;
    const f = (this.width + this.height) / 2.0;
    const cx = this.width / 2.0;
    const cy = this.height / 2.0;
    this.view = new DOMMatrix([
      1, 0, 0 , 0,
      0, 1, 0 , 0,
      0, 0, 1 , 0,
      0, 0, 10, 1
    ]);
    this.projection = new DOMMatrix([
      f , 0 , 0, 0,
      0 , f , 0, 0,
      cx, cy, 1, 1,
      0 , 0 , 0, 0
    ]);
    this.color = color;
  }
  worldToScreen(points: number[][]): DOMPoint[] {
    return points
      .map(point => new DOMPoint(point[0], point[1], point[2], 1))
      .map(point => this.view.transformPoint(point))
      .map(point => this.projection.transformPoint(point))
      .map(point => { point.x /= point.w; point.y /= point.w; return point; });
  }
  draw(cam: CameraModel): void {
    if (cam === (this as VirtualCameraModel)) { return; }
    const width = 1;
    const height = this.height * width / this.width;
    const focalLength = -this.projection.m11 * width / this.width;
    const viewInverse = vector.normalizeScale(this.view).inverse();
    const center = viewInverse.transformPoint(new DOMPoint(0.0, 0.0, 0.0, 1.0));
    const p1 = viewInverse.transformPoint(new DOMPoint(-width, -height, -focalLength));  // left top
    const p2 = viewInverse.transformPoint(new DOMPoint(+width, -height, -focalLength));  // right top
    const p3 = viewInverse.transformPoint(new DOMPoint(+width, +height, -focalLength));  // right bottom
    const p4 = viewInverse.transformPoint(new DOMPoint(-width, +height, -focalLength));  // left bottom
    cam.line3d(center.x, center.y, center.z, p1.x, p1.y, p1.z, 2, this.color);
    cam.line3d(center.x, center.y, center.z, p2.x, p2.y, p2.z, 2, this.color);
    cam.line3d(center.x, center.y, center.z, p3.x, p3.y, p3.z, 2, this.color);
    cam.line3d(center.x, center.y, center.z, p4.x, p4.y, p4.z, 2, this.color);
    cam.line3d(p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, 2, this.color);
    cam.line3d(p2.x, p2.y, p2.z, p3.x, p3.y, p3.z, 2, this.color);
    cam.line3d(p3.x, p3.y, p3.z, p4.x, p4.y, p4.z, 2, this.color);
    cam.line3d(p4.x, p4.y, p4.z, p1.x, p1.y, p1.z, 2, this.color);
  }
}

class CameraModel extends VirtualCameraModel {
  canvas: HTMLCanvasElement;
  context2d: CanvasRenderingContext2D;
  scale: number;
  constructor(canvas: HTMLCanvasElement, color: string = "#77A9B0") {
    super(
      Number(getComputedStyle(canvas).width.replace(/px$/, ""))  * devicePixelRatio,
      Number(getComputedStyle(canvas).height.replace(/px$/, "")) * devicePixelRatio,
      color
    );
    this.canvas = canvas;
    this.canvas.width  = this.width;
    this.canvas.height = this.height;
    this.context2d = canvas.getContext("2d")!;
    this.scale = 2.0;
  }
  clear(): void { this.context2d.clearRect(0, 0, this.canvas.width, this.canvas.height); }
  point2d(x: number, y: number, hex: string = "#77A9B0"): void {
    const ctx = this.context2d;
    ctx.save();
    try {
      ctx.fillStyle = hex;
      ctx.fillRect(x - 4 * this.scale, y - 1 * this.scale, 8 * this.scale, 2 * this.scale);
      ctx.fillRect(x - 1 * this.scale, y - 4 * this.scale, 2 * this.scale, 8 * this.scale);
    }
    finally {
      ctx.restore();
    }
  }
  point3d(x: number, y: number, z: number, hex: string = "#77A9B0"): void {
    const point = this.worldToScreen([[x, y, z]])[0];
    if (point.w <= 0.0) { return; }
    this.point2d(point.x, point.y, hex);
  }
  line2d(x1: number, y1: number, x2: number, y2: number, width: number = 4, hex: string = "#77A9B0"): void {
    const ctx = this.context2d;
    ctx.save();
    try {
      ctx.lineWidth = width * this.scale;
      ctx.strokeStyle = hex;
      ctx.beginPath();
      ctx.moveTo(x1, y1);
      ctx.lineTo(x2, y2);
      ctx.stroke();
    }
    finally {
      ctx.restore();
    }
  }
  line3d(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number, width: number = 4, hex: string = "#77A9B0"): void {
    const points = this.worldToScreen([
      [x1, y1, z1],
      [x2, y2, z2]
    ]);
    if (points.some(point => (point.w <= 0.0))) { return; }
    this.line2d(points[0].x, points[0].y, points[1].x, points[1].y, width, hex);
  }
  update(): void {
    setTimeout(() => {
      this.clear();
      this.scene!.models.forEach(model => {
        model.draw(this);
      });
    }, 0);
  }
}

class CameraModelWithMouse extends CameraModel {
  pointerList: Map<number, [number, number]>;
  constructor(canvas: HTMLCanvasElement, color: string = "#77A9B0") {
    super(canvas, color);
    this.pointerList = new Map();
    canvas.addEventListener("pointerdown", (e: PointerEvent) => {
      this.pointerList.set(e.pointerId, [e.offsetX, e.offsetY]);
      const callback = (e: PointerEvent) => {
        const offset: [number, number] = [e.offsetX, e.offsetY];
        const preOffset = this.pointerList.get(e.pointerId)!;
        const movement = offset.map((num, i) => (num - preOffset[i]));
        this.pointerList.set(e.pointerId, offset);
        this.mouseController(movement, e.shiftKey, e.ctrlKey);
      };
      window.addEventListener("pointermove", callback);
      window.addEventListener("pointerup", (e: PointerEvent) => {
        window.removeEventListener("pointermove", callback);
        this.pointerList.delete(e.pointerId);
        document.body.style.userSelect = "auto";
      });
      document.body.style.userSelect = "none";
    });
  }
  mouseController([movementX, movementY]: number[], shiftKey: boolean, ctrlKey: boolean): void {
    if (shiftKey) {
      this.view.m41 += movementX / 100.0;
      this.view.m42 += movementY / 100.0;
    }
    else if (ctrlKey) {
      const scale = 1.0 - movementY / 100.0;
      this.view.m43 -= 10;
      this.view.scaleSelf(scale, scale, scale);
      this.view.m43 += 10;
    }
    else {
      const move = new DOMPoint(-movementY, movementX, 0.0, 1.0);
      const length = vector.length(move);
      const rotate = vector.rotate(move, length / 100.0);
      this.view.m43 -= 10;
      this.view.preMultiplySelf(rotate);
      this.view.m43 += 10;
    }
    this.scene!.update();
  }
}

class AxisModel extends Model {
  draw(cam: CameraModel): void {
    cam.line3d(0, 0, 0, 1, 0, 0, 4, "#FA8CBB");
    cam.line3d(0, 0, 0, 0, 1, 0, 4, "#73E4FA");
    cam.line3d(0, 0, 0, 0, 0, 1, 4, "#F9F098");
  }
}

class PointsModel extends Model {
  points: number[][];
  color: string;
  constructor(color: string = "#77A9B0", points: number[][] = []) {
    super();
    this.points = points;
    this.color = color;
  }
  draw(cam: CameraModel): void {
    for(const point of this.points) {
      cam.point3d(point[0], point[1], point[2], this.color);
    }
  }
  updatePoints(points: number[][]): void {
    this.points = points;
    this.scene!.update();
  }
}

class LinesModel extends Model {
  points: number[][];
  lines: number[][];
  color: string;
  constructor(color: string = "#77A9B0", points: number[][] = [], lines: number[][] = []) {
    super();
    this.points = points;
    this.lines = lines;
    this.color = color;
  }
  draw(cam: CameraModel): void {
    for(const line of this.lines) {
      const points = this.points;
      const p = [points[line[0]], points[line[1]], points[line[2]]].map(q => q.slice(0, 3));
      cam.line3d(p[0][0], p[0][1], p[0][2], p[1][0], p[1][1], p[1][2], 1, this.color);
      cam.line3d(p[1][0], p[1][1], p[1][2], p[2][0], p[2][1], p[2][2], 1, this.color);
      cam.line3d(p[2][0], p[2][1], p[2][2], p[0][0], p[0][1], p[0][2], 1, this.color);
    }
  }
  updatePoints(points: number[][], lines: number[][]): void {
    this.points = points;
    this.lines = lines;
    this.scene!.update();
  }
}

export default {
  Scene,
  Model,
  VirtualCameraModel,
  CameraModel,
  CameraModelWithMouse,
  AxisModel,
  PointsModel,
  LinesModel,
};
