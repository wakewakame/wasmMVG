FROM node:24-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        clang \
        make \
        cmake \
        python3 \
        xz-utils \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*
COPY lib/emsdk /emsdk
RUN VERSION="$(node -p 'JSON.parse(require("fs").readFileSync("/emsdk/emscripten-releases-tags.json","utf8")).aliases.latest')" \
    && /emsdk/emsdk install "${VERSION}" \
    && /emsdk/emsdk activate "${VERSION}" \
    && chmod -R a+rwX /emsdk
ENV PATH="${PATH}:/emsdk:/emsdk/upstream/emscripten"

WORKDIR /workdir
CMD ["bash"]
