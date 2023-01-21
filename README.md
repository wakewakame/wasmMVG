# wasmMVG

wasm を用いて [openMVG](https://github.com/openMVG/openMVG) をブラウザで動かす試み。

# 使い方

```bash
./docker/docker.sh
./build.sh init
./build.sh build wasm RELEASE
python -m http.server
```

`http://localhost:8000` にアクセスし、画像を選択して `compute` ボタンを押す。

