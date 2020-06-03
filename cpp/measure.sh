#!/bin/bash

# To use: cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_CONCURRENT=ON -DTBB_DIR=/home/kovi/Downloads/tbb/cmake -DNNODES_MILLION=100 -DNEDGES_MILLION=200
# echo "Generating dataset..."
# ./generate-relabel-data

echo "Measuring..."

echo "libcuckoo map"
./relabel-libcuckoo
echo

echo "growt map"
./relabel-growt
echo

echo "Unordered map"
./relabel-unordered-map
echo

echo "Unordered map single thread"
./relabel-unordered-map-single-thread
echo

echo "LAGraph"
./relabel
echo
