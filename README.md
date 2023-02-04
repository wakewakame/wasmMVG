# wasmMVG

wasm を用いて [openMVG](https://github.com/openMVG/openMVG) をブラウザで動かす試み。

[demo](https://wakewakame.github.io/wasmMVG/)

# 使い方

```bash
./docker/docker.sh
./build.sh init
./build.sh build wasm RELEASE
python -m http.server
```

`http://localhost:8000` にアクセスし、画像を選択して `compute` ボタンを押す。
`wasmMVG/lib/ImageDataset_SceauxCastle/images` にサンプル画像があるので、これを利用することもできる。

