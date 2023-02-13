FROM ubuntu:22.04

RUN useradd -m -s /bin/bash user
WORKDIR /home/user/app

RUN apt update -y
RUN apt upgrade -y
RUN apt install -y git
RUN apt install -y clang
RUN apt install -y cmake
RUN apt install -y valgrind
RUN apt install -y xz-utils
RUN apt install -y lbzip2
RUN apt install -y npm
RUN apt install -y curl
RUN apt install -y jq
RUN npm install -g n
RUN n install 18.13.0

USER user
