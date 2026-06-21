# wasmMVG

wasm を用いて [openMVG](https://github.com/openMVG/openMVG) をブラウザで動かす試み。

# インストール

`npm install https://github.com/wakewakame/wasmMVG/releases/download/0.0.4/wasmMVG-0.0.4.tgz`

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

`ビルド` の成果物を使ったサンプルデモが `html/index.html` にある。
以下のように適当な HTTP サーバーを立てて <http://localhost:8000/html/index.html> にアクセスすると、Stanford Bunny を 5 つの視点から三次元復元したモデルが表示される。

```sh
python3 -m http.server 8000
```
