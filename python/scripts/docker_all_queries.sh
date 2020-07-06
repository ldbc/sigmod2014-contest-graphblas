#!/usr/bin/env bash
docker run --rm --user root -e NB_UID=$(id -u) -e NB_GID=$(id -g) \
  -v "$(pwd)":/home/jovyan/sigmod2014-pc \
  -it ftsrg/pygraphblas-notebook \
  bash -c "~/sigmod2014-pc/python/scripts/run_all_queries.sh $*"
