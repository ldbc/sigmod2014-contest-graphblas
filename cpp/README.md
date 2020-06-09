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

git clone --depth 1 --branch master https://github.com/DrTimothyAldenDavis/GraphBLAS/
cd GraphBLAS
make && sudo make install && sudo ldconfig
cd ..

git clone https://github.com/GraphBLAS/LAGraph
cd LAGraph
make && sudo make install && sudo ldconfig
cd ..
```

## Getting started

Put [converted CSVs](../README.md#preprocessing-the-provided-data-sets) to `../csvs/o1k/` or set `$CsvPath` environmental variable.

To build and test the query implementation on o1k (original contest data set with 1k persons) with [test queries](query-parameters.cpp), run:

```bash
rm -rf cmake-build-release
mkdir cmake-build-release
pushd cmake-build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./sigmod2014pc_cpp
popd
```

To only build run this from the repo's root:
```bash
pushd cpp && rm -rf cmake-build-release && mkdir cmake-build-release && pushd cmake-build-release && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j$(nproc) ; popd ; popd
```

## Generate new query parameters

Set `$CsvPath` environment variable to the data set.
To generate query parameters, set `$ParamsPath`, then run:

```bash
./paramgen
# running it with default values is the same as:
CsvPath=../../csvs/o1k/ ParamsPath=../../params/o1k/ ./paramgen
```
