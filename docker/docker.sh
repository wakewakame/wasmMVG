#!/bin/sh
set -eu
SCRIPT_DIR="$(cd "$(dirname "$(readlink "$0" || echo "$0")")" && pwd)"

docker build \
    -t wasm_mvg \
    $SCRIPT_DIR
docker run \
  -it --rm \
  --user $(id -u):$(id -g) \
  --mount type=bind,source="$(realpath "${SCRIPT_DIR}/../")",target=/workdir \
  -p 8000:8000 \
  wasm_mvg

