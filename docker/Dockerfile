FROM ubuntu:22.04

RUN useradd -m -s /bin/bash user
WORKDIR /home/user/app

RUN apt update -y
RUN apt upgrade -y
RUN apt install -y git
RUN apt install -y clang
RUN apt install -y cmake
RUN apt install -y nodejs
RUN apt install -y npm
RUN apt install -y jq

USER user
