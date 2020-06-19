#!/usr/bin/env bash
docker run --rm --user root -e NB_UID=$(id -u) -e NB_GID=$(id -g) \
  -v "$(pwd)":/home/jovyan/sigmod2014-pc \
  -it graphblas/pygraphblas-notebook:custom \
  bash -c 'cd ~/sigmod2014-pc/python ; python3 main.py --mode from_file'
