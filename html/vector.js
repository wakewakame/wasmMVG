const length = (point) => {
	return Math.sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
}

const normal = (point) => {
	const length_ = length(point);
	if (length_ === 0.0) return null;
	return new DOMPoint(point.x / length_, point.y / length_, point.z / length_, 1.0);
};

const rotate = (axis, rad) => {
	const naxis = normal(axis);
	if (naxis === null) {
		return new DOMMatrix([
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		]);
	}
	const x = naxis.x * Math.sin(rad / 2.0);
	const y = naxis.y * Math.sin(rad / 2.0);
	const z = naxis.z * Math.sin(rad / 2.0);
	const w = Math.cos(rad / 2.0);
	return new DOMMatrix([
		1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y), 0,
		2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x), 0,
		2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y), 0,
		0, 0, 0, 1
	]);
};

// 平行移動行列x拡大縮小行列x回転行列 を 平行移動行列x回転行列 へ変換する関数
const normalizeScale = (matrix) => {
	const determinant3x3 =
		matrix.m11 * matrix.m22 * matrix.m33 +
		matrix.m21 * matrix.m32 * matrix.m13 +
		matrix.m31 * matrix.m12 * matrix.m23 -
		matrix.m31 * matrix.m22 * matrix.m13 -
		matrix.m21 * matrix.m12 * matrix.m33 -
		matrix.m11 * matrix.m32 * matrix.m23;
	const scale = 1.0 / Math.pow(determinant3x3, 1/3);
	return new DOMMatrix([
		matrix.m11 * scale, matrix.m12 * scale, matrix.m13 * scale, 0,
		matrix.m21 * scale, matrix.m22 * scale, matrix.m23 * scale, 0,
		matrix.m31 * scale, matrix.m32 * scale, matrix.m33 * scale, 0,
		matrix.m41 * scale, matrix.m42 * scale, matrix.m43 * scale, 1
	]);
};

const matToJson = (matrix) => {
	return JSON.stringify(Array.from(matrix.toFloat64Array()));
};

const matFromJson = (json) => {
	return DOMMatrix.fromFloat64Array(new Float64Array(JSON.parse(json)));
};

const matToOpencvIntrinsics = (projection) => {
	return [
		[projection.m11, 0, projection.m31],
		[0, projection.m22, projection.m32],
		[0, 0, 1]
	];
};

export default {
	length: length,
	normal: normal,
	rotate: rotate,
	normalizeScale: normalizeScale,
	matToJson: matToJson,
	matFromJson: matFromJson,
	matToOpencvIntrinsics: matToOpencvIntrinsics,
};
