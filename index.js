const main = () => {
	const fileElem = document.getElementById("file");
	const computeElem = document.getElementById("compute");
	FS.mkdir('/home/web_user/images');
	fileElem.addEventListener("change", () => {
		let uploadProgress = 0;
		Array.from(fileElem.files).forEach((file, index) => {
			const reader = new FileReader();
			reader.onload = () => {
				const bytes = new Uint8Array(reader.result);
				FS.writeFile(`/home/web_user/images/${file.name}`, bytes);
				console.log(`${++uploadProgress} / ${fileElem.files.length}: ${file.name}`);
			};
			reader.readAsArrayBuffer(file);
		});
	});
	computeElem.addEventListener("click", () => {
		Module.callMain(['/home/web_user']);
		const result = FS.readFile('/home/web_user/sparse_model/cloud_and_poses.ply');
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
	// 読み込んでいきなり main 関数を実行しないようにする
	window.Module = { noInitialRun: true };

	// wasm 読み込み
	const scriptElem = document.createElement("script");
	scriptElem.src = "./build/wasm/RELEASE/main.js";
	scriptElem.addEventListener('load', (_) => {
		Module.addOnPostRun(() => {
			main();
		});
	});
	document.body.appendChild(scriptElem);
});
