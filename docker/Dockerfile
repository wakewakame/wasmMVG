FROM node:24-slim

RUN apt update -y
RUN apt upgrade -y

# for build
RUN apt install -y git
RUN apt install -y clang
RUN apt install -y cmake
RUN apt install -y jq
RUN apt install -y xz-utils

WORKDIR /workdir
CMD ["bash"]
