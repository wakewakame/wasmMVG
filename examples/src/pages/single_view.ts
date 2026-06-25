import wasmMVG from 'wasmMVG';
import type { Module, Pose, Vec3 } from 'wasmMVG';

const canvas = document.getElementById("preview") as HTMLCanvasElement;
const ctx = canvas.getContext("2d")!;
const fSlider = document.getElementById("f-slider") as HTMLInputElement;
const cxSlider = document.getElementById("cx-slider") as HTMLInputElement;
const cySlider = document.getElementById("cy-slider") as HTMLInputElement;
const fDisplay = document.getElementById("f-value") as HTMLSpanElement;
const cxDisplay = document.getElementById("cx-value") as HTMLSpanElement;
const cyDisplay = document.getElementById("cy-value") as HTMLSpanElement;
const imageInput = document.getElementById("image-input") as HTMLInputElement;

const EDGES: [number, number][] = [
  [0, 1], [1, 2], [2, 3], [3, 0],
  [4, 5], [5, 6], [6, 7], [7, 4],
  [0, 4], [1, 5], [2, 6], [3, 7],
];

const CUBE: Vec3[] = [
  [-1, -1, -1], [1, -1, -1], [1, 1, -1], [-1, 1, -1],
  [-1, -1,  1], [1, -1,  1], [1, 1,  1], [-1, 1,  1],
];

let wasm: Module | null = null;
let bgImage: HTMLImageElement | null = null;
let f = 560;
let cx = 320;
let cy = 240;
let targets: ([number, number] | null)[] = new Array<null>(8).fill(null);

function makeInitialPose(): Pose {
  const ay = Math.PI / 6, ax = Math.PI / 9;
  const c1 = Math.cos(ay), s1 = Math.sin(ay);
  const c2 = Math.cos(ax), s2 = Math.sin(ax);
  return {
    rotation: [
      [c1, 0, -s1],
      [s1 * s2, c2, c1 * s2],
      [s1 * c2, -s2, c1 * c2],
    ],
    translation: [0, 0, 5],
  };
}

let pose: Pose = makeInitialPose();

// --- Projection ---

function projectPoint(p: Vec3): [number, number] | null {
  const R = pose.rotation; // R[col][row] = R(row, col) (列優先)
  const t = pose.translation;
  const camX = R[0][0] * p[0] + R[1][0] * p[1] + R[2][0] * p[2] + t[0];
  const camY = R[0][1] * p[0] + R[1][1] * p[1] + R[2][1] * p[2] + t[1];
  const camZ = R[0][2] * p[0] + R[1][2] * p[1] + R[2][2] * p[2] + t[2];
  if (camZ <= 0) return null;
  return [f * camX / camZ + cx, f * camY / camZ + cy];
}

function solvePose(): void {
  if (!wasm) return;
  const indices: number[] = [];
  for (let i = 0; i < targets.length; i++) {
    if (targets[i]) indices.push(i);
  }
  if (indices.length < 1) return;

  // 点数に応じてカメラフレームの自由度を制限
  // bit0-2: 回転 (wx, wy, wz), bit3-5: 並進 (tx, ty, tz)
  // bit0-2: 回転 (wx, wy, wz), bit3-5: 並進 (tx, ty, tz)
  const dofMask = indices.length === 1 ? 0x18   // tx, ty のみ (スクリーン平行移動)
               : indices.length === 2 ? 0x3C    // wz, tx, ty, tz (画面内回転 + 平行移動 + 奥行き)
               : 0x3F;                          // 全 6 自由度

  const intrinsic = {
    type: "PINHOLE_CAMERA_RADIAL3" as const,
    width: canvas.width, height: canvas.height,
    focal: f, ppx: cx, ppy: cy,
    k1: 0, k2: 0, k3: 0,
  };
  const pts2d = new Float64Array(indices.length * 2);
  const pts3d = new Float64Array(indices.length * 3);
  for (let i = 0; i < indices.length; i++) {
    const vi = indices[i];
    pts2d[i * 2] = targets[vi]![0];
    pts2d[i * 2 + 1] = targets[vi]![1];
    pts3d[i * 3] = CUBE[vi][0];
    pts3d[i * 3 + 1] = CUBE[vi][1];
    pts3d[i * 3 + 2] = CUBE[vi][2];
  }

  const result = wasm.refinePose(intrinsic, pts2d, pts3d, pose, 20, dofMask);
  if (result.result === "ok") {
    pose = result.value;
  }
}

// --- Coordinate helpers ---

function cssToCanvas(cssX: number, cssY: number): [number, number] {
  return [
    cssX * canvas.width / canvas.clientWidth,
    cssY * canvas.height / canvas.clientHeight,
  ];
}

function dScale(): number {
  return canvas.width / canvas.clientWidth;
}

// --- Render ---

function render(): void {
  const s = dScale();
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  if (bgImage) ctx.drawImage(bgImage, 0, 0);

  // Edges (逆算座標を結ぶ)
  ctx.strokeStyle = "#77A9B0";
  ctx.lineWidth = 2 * s;
  for (const [i, j] of EDGES) {
    const p1 = projectPoint(CUBE[i]);
    const p2 = projectPoint(CUBE[j]);
    if (!p1 || !p2) continue;
    ctx.beginPath();
    ctx.moveTo(p1[0], p1[1]);
    ctx.lineTo(p2[0], p2[1]);
    ctx.stroke();
  }

  // Vertices (逆算座標)
  const r = 5 * s;
  for (let i = 0; i < CUBE.length; i++) {
    const p = projectPoint(CUBE[i]);
    if (!p) continue;
    ctx.fillStyle = targets[i] ? "#FF3090" : "#77A9B0";
    ctx.beginPath();
    ctx.arc(p[0], p[1], r, 0, Math.PI * 2);
    ctx.fill();
  }

  // 目標座標と逆算座標を結ぶ線
  ctx.strokeStyle = "#F9F098";
  ctx.lineWidth = 1.5 * s;
  ctx.setLineDash([4 * s, 4 * s]);
  for (let i = 0; i < targets.length; i++) {
    const t = targets[i];
    if (!t) continue;
    const p = projectPoint(CUBE[i]);
    if (!p) continue;
    ctx.beginPath();
    ctx.moveTo(t[0], t[1]);
    ctx.lineTo(p[0], p[1]);
    ctx.stroke();
  }
  ctx.setLineDash([]);

  // Target crosshairs (目標座標)
  const cr = 8 * s;
  const ext = 4 * s;
  ctx.strokeStyle = "#F9F098";
  ctx.lineWidth = 2 * s;
  for (let i = 0; i < targets.length; i++) {
    const t = targets[i];
    if (!t) continue;
    const [tx, ty] = t;
    ctx.beginPath();
    ctx.arc(tx, ty, cr, 0, Math.PI * 2);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(tx - cr - ext, ty);
    ctx.lineTo(tx + cr + ext, ty);
    ctx.moveTo(tx, ty - cr - ext);
    ctx.lineTo(tx, ty + cr + ext);
    ctx.stroke();
  }
}

// --- Interaction ---

const CLICK_THRESHOLD = 4;
let activeIndex = -1;
let isDragging = false;
let downX = 0;
let downY = 0;

function findNearest(cssX: number, cssY: number, points: ([number, number] | null)[]): number {
  const [mx, my] = cssToCanvas(cssX, cssY);
  const threshold = 14 * dScale();
  let minDist = threshold;
  let closest = -1;
  for (let i = 0; i < points.length; i++) {
    const p = points[i];
    if (!p) continue;
    const d = Math.hypot(p[0] - mx, p[1] - my);
    if (d < minDist) { minDist = d; closest = i; }
  }
  return closest;
}

function projectedVertices(): ([number, number] | null)[] {
  return CUBE.map(v => projectPoint(v));
}

canvas.addEventListener("pointerdown", (e: PointerEvent) => {
  let idx = findNearest(e.offsetX, e.offsetY, targets);
  if (idx < 0) idx = findNearest(e.offsetX, e.offsetY, projectedVertices());
  if (idx >= 0) {
    activeIndex = idx;
    isDragging = false;
    downX = e.offsetX;
    downY = e.offsetY;
    canvas.setPointerCapture(e.pointerId);
  }
});

canvas.addEventListener("pointermove", (e: PointerEvent) => {
  if (activeIndex < 0) {
    const hit = findNearest(e.offsetX, e.offsetY, targets) >= 0
      || findNearest(e.offsetX, e.offsetY, projectedVertices()) >= 0;
    canvas.style.cursor = hit ? "pointer" : "default";
    return;
  }

  if (!isDragging && Math.hypot(e.offsetX - downX, e.offsetY - downY) > CLICK_THRESHOLD) {
    isDragging = true;
    if (!targets[activeIndex]) {
      const p = projectPoint(CUBE[activeIndex]);
      if (p) targets[activeIndex] = [p[0], p[1]];
    }
  }

  if (isDragging && targets[activeIndex]) {
    const [mx, my] = cssToCanvas(e.offsetX, e.offsetY);
    targets[activeIndex] = [mx, my];
    solvePose();
    canvas.style.cursor = "grabbing";
    render();
  }
});

canvas.addEventListener("pointerup", () => {
  if (activeIndex >= 0 && !isDragging) {
    if (targets[activeIndex]) {
      targets[activeIndex] = null;
    } else {
      const p = projectPoint(CUBE[activeIndex]);
      if (p) targets[activeIndex] = [p[0], p[1]];
    }
    solvePose();
    render();
  }
  activeIndex = -1;
  isDragging = false;
  canvas.style.cursor = "default";
});

canvas.addEventListener("lostpointercapture", () => {
  activeIndex = -1;
  isDragging = false;
  canvas.style.cursor = "default";
  render();
});

// --- Controls ---

function syncSliders(): void {
  fSlider.value = String(Math.round(f));
  cxSlider.value = String(Math.round(cx));
  cySlider.value = String(Math.round(cy));
  fDisplay.textContent = String(Math.round(f));
  cxDisplay.textContent = String(Math.round(cx));
  cyDisplay.textContent = String(Math.round(cy));
}

fSlider.addEventListener("input", () => {
  f = Number(fSlider.value);
  fDisplay.textContent = fSlider.value;
  solvePose();
  render();
});

cxSlider.addEventListener("input", () => {
  cx = Number(cxSlider.value);
  cxDisplay.textContent = cxSlider.value;
  solvePose();
  render();
});

cySlider.addEventListener("input", () => {
  cy = Number(cySlider.value);
  cyDisplay.textContent = cySlider.value;
  solvePose();
  render();
});

imageInput.addEventListener("change", () => {
  const file = imageInput.files?.[0];
  if (!file) return;
  const img = new Image();
  img.onload = () => {
    bgImage = img;
    canvas.width = img.naturalWidth;
    canvas.height = img.naturalHeight;

    f = (img.naturalWidth + img.naturalHeight) / 2;
    cx = img.naturalWidth / 2;
    cy = img.naturalHeight / 2;

    fSlider.max = String(Math.round(Math.max(3000, f * 3)));
    cxSlider.max = String(img.naturalWidth);
    cySlider.max = String(img.naturalHeight);

    syncSliders();
    targets = new Array<null>(8).fill(null);
    pose = makeInitialPose();
    render();
  };
  img.src = URL.createObjectURL(file);
});

// --- Init ---
syncSliders();
render();

wasmMVG({ noInitialRun: true }).then(m => { wasm = m; });
