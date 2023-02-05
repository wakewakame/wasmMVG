# wasmMVG

wasm を用いて [openMVG](https://github.com/openMVG/openMVG) をブラウザで動かす試み。

[demo](https://wakewakame.github.io/wasmMVG/)

# 使い方

```bash
git clone https://github.com/wakewakame/wasmMVG.git
cd wasmMVG
git submodule update --init --recursive
./docker/docker.sh
./build.sh init
./build.sh build wasm RELEASE
python3 -m http.server 8000
```

`http://localhost:8000` にアクセスし、画像を選択して `compute` ボタンを押す。
`wasmMVG/lib/ImageDataset_SceauxCastle/images` にサンプル画像があるので、これを利用することもできる。

