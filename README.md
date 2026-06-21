# wasmMVG

wasm を用いて [openMVG](https://github.com/openMVG/openMVG) をブラウザで動かす試み。

# インストール

`npm install https://github.com/wakewakame/wasmMVG/releases/download/0.0.5/wasmMVG-0.0.5.tgz`

# ビルド

```sh
git clone https://github.com/wakewakame/wasmMVG.git
cd wasmMVG
git submodule update --init --recursive
docker build -t wasm_mvg .
docker run --rm \
    --user "$(id -u):$(id -g)" \
    --mount "type=bind,source=$(pwd),target=/workdir" \
    wasm_mvg make wasm-release
```

実行が完了すると `build/wasm/RELEASE/wasmMVG-X.X.X.tgz` が生成される。
これを `npm install /path/to/wasmMVG-X.X.X.tgz` でインストールして利用できる。

# デモ

`ビルド` の成果物を使ったサンプルデモが `./examples/` にある。
以下のコマンドでローカルサーバを立ち上げて、ブラウザで `http://localhost:3000` にアクセスするとデモが動作する。

```sh
# 先に `ビルド` の手順を実行して build/wasm/RELEASE/wasmMVG-X.X.X.tgz を生成しておくこと
cd examples
npm i
npm run dev
```
