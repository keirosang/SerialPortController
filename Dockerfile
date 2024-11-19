FROM ubuntu:20.04

# 安装必要的工具
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    nlohmann-json3-dev \
    g++-multilib

# 设置工作目录
WORKDIR /app

# 复制源代码
COPY . .

# 编译
RUN mkdir build && cd build && \
    cmake .. && \
    make 