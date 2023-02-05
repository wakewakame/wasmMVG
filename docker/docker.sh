#!/bin/sh
set -eu
SCRIPT_DIR="$(cd "$(dirname "$(readlink "$0" || echo "$0")")" && pwd)"

docker build \
    -t wasm_mvg \
    --platform linux/x86_64 \
    $SCRIPT_DIR
docker run \
  -it --rm \
  --platform linux/x86_64 \
  --mount type=bind,source="$(realpath "${SCRIPT_DIR}/../")",target=/home/user/app \
  -p 8000:8000 \
  wasm_mvg

