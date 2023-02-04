const main = () => {
	const fileElem = document.getElementById("file");
	const computeElem = document.getElementById("compute");
	FS.mkdir('/home/web_user/images');
	fileElem.addEventListener("change", () => {
		let uploadProgress = 0;
		Array.from(fileElem.files).forEach((file) => {
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
	window.Module = {
		// wasm の読み込みが完了したときのコールバック
		onRuntimeInitialized: () => { main(); },
		// main 関数の自動実行を無効化
		noInitialRun: true
	};

	// wasm 読み込み
	const scriptElem = document.createElement("script");
	scriptElem.src = "./build/wasm/RELEASE/main.js";
	document.body.appendChild(scriptElem);
});
