# wasmMVG

wasm を用いて [openMVG](https://github.com/openMVG/openMVG) をブラウザで動かす試み。

# 使い方

## wasm 向けビルド

```bash
./docker/docker.sh
./build.sh init
./build.sh build wasm DEBUG
./build.sh run wasm DEBUG
```

## ローカル向けビルド

```bash
./docker/docker.sh
./build.sh init
./build.sh build native DEBUG
./build.sh run native DEBUG
```

