FROM ubuntu:20.04

# to prevent tzdata from requiring user input
# https://askubuntu.com/a/1013396/415610
ENV DEBIAN_FRONTEND=noninteractive

RUN apt update
RUN apt install -y git gcc g++ cmake
RUN apt install -y libgraphblas3
RUN ln -s /usr/lib/x86_64-linux-gnu/libgraphblas.so.3 /usr/lib/x86_64-linux-gnu/libgraphblas.so

WORKDIR /opt
RUN git clone --depth 1 --branch v3.2.0 https://github.com/DrTimothyAldenDavis/GraphBLAS
RUN git clone https://github.com/GraphBLAS/LAGraph

WORKDIR /opt/LAGraph
RUN cmake .
RUN JOBS=$(nproc) make install
