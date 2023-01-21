const save = (filename, arrayBuffer) => {
	const ptr = Module._malloc(arrayBuffer.byteLength);
	Module.HEAPU8.set(new Uint8Array(arrayBuffer), ptr);
	const result = Module.ccall(
		'save', 'number', ['string', 'number', 'number'],
		[filename, ptr, arrayBuffer.byteLength]
	);
	Module._free(ptr);
	return result;
};

const load = (filename) => {
	const size = Module.ccall('load', 'number', ['string', 'number'], [filename, 0]);
	if (size === -1) { return null; }
	const ptr = Module._malloc(size);
	Module.ccall('load', 'number', ['string', 'number'], [filename, ptr]);
	const arrayBuffer = Module.HEAPU8.subarray(ptr, ptr + size).slice(0);
	Module._free(ptr);
	return arrayBuffer;
};

const main = () => {
	const fileElem = document.getElementById("file");
	const computeElem = document.getElementById("compute");
	Module.ccall('makedir', 'number', ['string'], ['/home/web_user/images']);
	fileElem.addEventListener("change", () => {
		let uploadProgress = 0;
		Array.from(fileElem.files).forEach((file, index) => {
			const reader = new FileReader();
			reader.onload = () => {
				const bytes = reader.result;
				save(`/home/web_user/images/${file.name}`, bytes);
				console.log(`${++uploadProgress} / ${fileElem.files.length}: ${file.name}`);
			};
			reader.readAsArrayBuffer(file);
		});
	});
	computeElem.addEventListener("click", () => {
		Module.ccall('sfm', 'number', [], []);
		Module.ccall('ls', 'number', ['string'], ['/home/web_user']);
		const result = load('/home/web_user/sparse_model/cloud_and_poses.ply');
		const downloadUrl = URL.createObjectURL(new Blob([result.buffer]));
		const downloadUrlElem = document.createElement("a");
		downloadUrlElem.download = "cloud_and_poses.ply";
		downloadUrlElem.href = downloadUrl;
		downloadUrlElem.innerText = "download result";
		document.body.appendChild(downloadUrlElem);
		console.log(`result: ${downloadUrl}`);
	});
};

document.addEventListener("DOMContentLoaded", () => {
	const scriptElem = document.createElement("script");
	scriptElem.src = "./build/wasm/RELEASE/main.js";
	scriptElem.addEventListener('load', (_) => {
		Module.addOnPostRun(() => {
			main();
		});
	});
	document.body.appendChild(scriptElem);
});
