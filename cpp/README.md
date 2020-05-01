# C++ implementation

## Prerequisites

Install CMake (the package in Ubuntu 18.04 satisfies the minimum version requirements):

```bash
sudo apt install cmake
```

Make sure you do not have GraphBLAS installed from APT:

```bash
sudo apt purge -y libgraphblas1
```

## Grab and compile dependencies

Install [SuiteSparse:GraphBLAS](https://github.com/DrTimothyAldenDavis/SuiteSparse) and [LAGraph](https://github.com/GraphBLAS/LAGraph/).

```bash
export JOBS=$(nproc)

git clone --depth 1 --branch v3.2.0 https://github.com/DrTimothyAldenDavis/GraphBLAS
cd GraphBLAS
make && sudo make install && sudo ldconfig
cd ..

git clone https://github.com/GraphBLAS/LAGraph
cd LAGraph
make && sudo make install && sudo ldconfig
cd ..
```

## Getting started

Put [converted CSVs](../README.md#preprocessing-the-provided-data-sets) to `../sf1k-converted/` or set `$ChangePath` environmental variable.

To build and run the code, run:

```bash
mkdir -p cmake-build-release
cd cmake-build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cd ..
./sigmod2014pc_cpp
```

## Generate new query parameters

Set `$ChangePath` environmental variable to the data set (default: `../sf1k-converted/`).

To generate query parameters to `$ParamsPath` (default: `../params/sf1k-converted/`) run:

```bash
./paramgen
```
