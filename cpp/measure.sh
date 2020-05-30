#!/bin/bash

# echo "Generating dataset..."
# ./generate-relabel-data

echo "Measuring..."

echo "LAGraph"
./relabel
echo

echo "Unordered map"
./relabel-unordered-map
echo

echo "Unordered map single thread"
./relabel-unordered-map-single-thread
echo

echo "Concurrent unordered map"
./relabel-concurrent-unordered-map
echo

echo "Concurrent unordered map single thread"
./relabel-concurrent-unordered-map-single-thread
echo

echo "Concurrent hash map"
./relabel-concurrent-hash-map
echo

echo "Concurrent hash map single thread"
./relabel-concurrent-hash-map-single-thread
echo

