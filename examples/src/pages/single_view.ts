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
const modelSelect = document.getElementById("model-select") as HTMLSelectElement;

// --- Model ---

interface Model {
  vertices: Vec3[];
  faces: [number, number, number][];
  edges: [number, number][];
  edgeToFaces: number[][];
}

function buildModel(vertices: Vec3[], faces: [number, number, number][]): Model {
  const edgeMap = new Map<string, { edge: [number, number]; faceIndices: number[] }>();
  for (let fi = 0; fi < faces.length; fi++) {
    const [a, b, c] = faces[fi];
    for (const [u, v] of [[a, b], [b, c], [c, a]]) {
      const key = Math.min(u, v) + "_" + Math.max(u, v);
      const entry = edgeMap.get(key);
      if (entry) entry.faceIndices.push(fi);
      else edgeMap.set(key, { edge: [u, v], faceIndices: [fi] });
    }
  }
  const edges: [number, number][] = [];
  const edgeToFaces: number[][] = [];
  for (const { edge, faceIndices } of edgeMap.values()) {
    edges.push(edge);
    edgeToFaces.push(faceIndices);
  }
  return { vertices, faces, edges, edgeToFaces };
}

const CUBE_MODEL: Model = buildModel(
  [
    [-1, -1, -1], [1, -1, -1], [1, 1, -1], [-1, 1, -1],
    [-1, -1,  1], [1, -1,  1], [1, 1,  1], [-1, 1,  1],
  ],
  [
    [0, 2, 1], [0, 3, 2],
    [4, 5, 6], [4, 6, 7],
    [0, 1, 5], [0, 5, 4],
    [2, 3, 7], [2, 7, 6],
    [0, 7, 3], [0, 4, 7],
    [1, 2, 6], [1, 6, 5],
  ],
);

function parsePLY(text: string): Model {
  const lines = text.split('\n');
  let vertexCount = 0;
  let faceCount = 0;
  let headerEnd = 0;
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i].trim();
    if (line.startsWith('element vertex')) vertexCount = parseInt(line.split(' ')[2]);
    if (line.startsWith('element face')) faceCount = parseInt(line.split(' ')[2]);
    if (line === 'end_header') { headerEnd = i + 1; break; }
  }
  const vertices: Vec3[] = [];
  for (let i = 0; i < vertexCount; i++) {
    const parts = lines[headerEnd + i].trim().split(/\s+/).map(Number);
    vertices.push([parts[0], parts[1], parts[2]]);
  }
  const faces: [number, number, number][] = [];
  for (let i = 0; i < faceCount; i++) {
    const parts = lines[headerEnd + vertexCount + i].trim().split(/\s+/).map(Number);
    faces.push([parts[1], parts[2], parts[3]]);
  }
  return buildModel(vertices, faces);
}

let bunnyModel: Model | null = null;

let wasm: Module | null = null;
let bgImage: HTMLImageElement | null = null;
let f = 560;
let cx = 320;
let cy = 240;
let model: Model = CUBE_MODEL;
let targets: ([number, number] | null)[] = new Array<null>(model.vertices.length).fill(null);
let vertexVisible: Uint8Array = new Uint8Array(model.vertices.length);

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

function resetModel(m: Model): void {
  model = m;
  targets = new Array<null>(model.vertices.length).fill(null);
  vertexVisible = new Uint8Array(model.vertices.length);
  pose = makeInitialPose();
  render();
}

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

function updateVisibility(): void {
  const faceVisible = model.faces.map(([a, b, c]) => {
    const pa = projectPoint(model.vertices[a]);
    const pb = projectPoint(model.vertices[b]);
    const pc = projectPoint(model.vertices[c]);
    if (!pa || !pb || !pc) return false;
    const cross = (pb[0] - pa[0]) * (pc[1] - pa[1]) - (pb[1] - pa[1]) * (pc[0] - pa[0]);
    return cross < 0;
  });
  vertexVisible = new Uint8Array(model.vertices.length);
  for (let fi = 0; fi < model.faces.length; fi++) {
    if (!faceVisible[fi]) continue;
    const [a, b, c] = model.faces[fi];
    vertexVisible[a] = vertexVisible[b] = vertexVisible[c] = 1;
  }
  for (let ei = 0; ei < model.edges.length; ei++) {
    if (!model.edgeToFaces[ei].some(fi => faceVisible[fi])) {
      edgeVisible[ei] = 0;
    } else {
      edgeVisible[ei] = 1;
    }
  }
}

let edgeVisible: Uint8Array = new Uint8Array(0);

function solvePose(): void {
  if (!wasm) return;
  const indices: number[] = [];
  for (let i = 0; i < targets.length; i++) {
    if (targets[i]) indices.push(i);
  }
  if (indices.length < 1) return;

  // 点数に応じてカメラフレームの自由度を制限
  //                                    tx ty tz wx wy wz
  const dofMask = indices.length === 1 ? 0b110000  // tx, ty
               : indices.length === 2 ? 0b111001  // tx, ty, tz, wz
               : 0b111111;                        // 全 6 自由度

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
    pts3d[i * 3] = model.vertices[vi][0];
    pts3d[i * 3 + 1] = model.vertices[vi][1];
    pts3d[i * 3 + 2] = model.vertices[vi][2];
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

  edgeVisible = new Uint8Array(model.edges.length);
  updateVisibility();

  // Back-face edges (半透明)
  ctx.strokeStyle = "rgba(0, 0, 0, 0.1)";
  ctx.lineWidth = 1 * s;
  for (let ei = 0; ei < model.edges.length; ei++) {
    if (edgeVisible[ei]) continue;
    const [i, j] = model.edges[ei];
    const p1 = projectPoint(model.vertices[i]);
    const p2 = projectPoint(model.vertices[j]);
    if (!p1 || !p2) continue;
    ctx.beginPath();
    ctx.moveTo(p1[0], p1[1]);
    ctx.lineTo(p2[0], p2[1]);
    ctx.stroke();
  }

  // Front-face edges
  ctx.strokeStyle = "#000000";
  for (let ei = 0; ei < model.edges.length; ei++) {
    if (!edgeVisible[ei]) continue;
    const [i, j] = model.edges[ei];
    const p1 = projectPoint(model.vertices[i]);
    const p2 = projectPoint(model.vertices[j]);
    if (!p1 || !p2) continue;
    ctx.beginPath();
    ctx.moveTo(p1[0], p1[1]);
    ctx.lineTo(p2[0], p2[1]);
    ctx.stroke();
  }

  // Vertices (表面のみ)
  const r = 3 * s;
  for (let i = 0; i < model.vertices.length; i++) {
    if (!vertexVisible[i]) continue;
    const p = projectPoint(model.vertices[i]);
    if (!p) continue;
    ctx.fillStyle = targets[i] ? "#FF3090" : "#000000";
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
    const p = projectPoint(model.vertices[i]);
    if (!p) continue;
    ctx.beginPath();
    ctx.moveTo(t[0], t[1]);
    ctx.lineTo(p[0], p[1]);
    ctx.stroke();
  }
  ctx.setLineDash([]);

  // Target crosshairs
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
  return model.vertices.map((v, i) => vertexVisible[i] ? projectPoint(v) : null);
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
      const p = projectPoint(model.vertices[activeIndex]);
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
      const p = projectPoint(model.vertices[activeIndex]);
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
  const url = URL.createObjectURL(file);
  img.onload = () => {
    URL.revokeObjectURL(url);
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
    targets = new Array<null>(model.vertices.length).fill(null);
    pose = makeInitialPose();
    render();
  };
  img.src = url;
});

modelSelect.addEventListener("change", async () => {
  if (modelSelect.value === "bunny") {
    if (!bunnyModel) {
      const res = await fetch("/example_images/model.ply");
      bunnyModel = parsePLY(await res.text());
    }
    resetModel(bunnyModel);
  } else {
    resetModel(CUBE_MODEL);
  }
});

// --- Init ---
syncSliders();
render();

wasmMVG({ noInitialRun: true }).then(m => { wasm = m; });
