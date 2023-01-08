# wasmMVG

wasm を用いて [openMVG](https://github.com/openMVG/openMVG) をブラウザで動かす試み。

# 動作確認済みビルド環境

- OS: Ubuntu 20.04
- CMake: 3.24.2
- node.js: v18.13.0

# 使い方

## wasm 向けビルド

```bash
./build.sh init wasm DEBUG
./build.sh build wasm DEBUG
./build.sh run wasm DEBUG
```

## ローカル向けビルド

```bash
./build.sh init native DEBUG
./build.sh build native DEBUG
./build.sh run native DEBUG
```

