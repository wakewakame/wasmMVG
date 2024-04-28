# wasmMVG

wasm を用いて [openMVG](https://github.com/openMVG/openMVG) をブラウザで動かす試み。

# インストール

`npm install https://github.com/wakewakame/wasmMVG/releases/download/0.0.3/wasmMVG-0.0.3.tgz`

# ビルド

```bash
git clone https://github.com/wakewakame/wasmMVG.git
cd wasmMVG
./docker/docker.sh
./build.sh init
./build.sh build wasm RELEASE
python3 -m http.server 8000
```

`http://localhost:8000/html/index.html` にアクセスすると Stanford Bunny を5つの視点から三次元復元したモデルが表示される。

また、 `/wasmMVG/build/wasm/RELEASE/wasmMVG-*.tgz` に npm パッケージが生成されるため、これを `npm install /path/to wasmMVG-*.tgz` することもできる。

