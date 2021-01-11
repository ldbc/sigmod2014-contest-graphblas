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
scripts/rebuild.sh
pushd cmake-build-release
./sigmod2014pc_cpp
popd
```

To only build run this from the repo's root:
```bash
cpp/scripts/rebuild.sh
```

To run all queries on size `o1k` (from the root folder):
```bash
cpp/cmake-build-release/sigmod2014pc_cpp csvs/o1k/ FILE <(cat params/o1k/query*.txt)
```

Prefix the build command with `PRINT_RESULTS=0` to set the environment variable if result and comment columns are not necessary.

## Generate new query parameters

Set `$CsvPath` environment variable to the data set.
To generate query parameters, set `$ParamsPath`, then run:

```bash
./paramgen
# running it with default values is the same as:
CsvPath=../../csvs/o1k/ ParamsPath=../../params/o1k/ ./paramgen
```

Generate all:
```bash
export CSVS_BASE_FOLDER=csvs/
export PARAMS_BASE_FOLDER=params/

CsvPath=$CSVS_BASE_FOLDER/o1k/ ParamsPath=$PARAMS_BASE_FOLDER/o1k/ cpp/cmake-build-release/paramgen
CsvPath=$CSVS_BASE_FOLDER/o10k/ ParamsPath=$PARAMS_BASE_FOLDER/o10k/ cpp/cmake-build-release/paramgen

CsvPath=$CSVS_BASE_FOLDER/p1k/ ParamsPath=$PARAMS_BASE_FOLDER/p1k/ cpp/cmake-build-release/paramgen
CsvPath=$CSVS_BASE_FOLDER/p10k/ ParamsPath=$PARAMS_BASE_FOLDER/p10k/ cpp/cmake-build-release/paramgen
CsvPath=$CSVS_BASE_FOLDER/p100k/ ParamsPath=$PARAMS_BASE_FOLDER/p100k/ cpp/cmake-build-release/paramgen
CsvPath=$CSVS_BASE_FOLDER/p1000k/ ParamsPath=$PARAMS_BASE_FOLDER/p1000k/ cpp/cmake-build-release/paramgen
```
