docker run --rm --user root -e NB_UID=$(id -u) -e NB_GID=$(id -g) \
  -v "$(pwd)":/home/jovyan/sigmod2014-pc \
  -it graphblas/pygraphblas-notebook:custom \
  bash -c 'cd ~/sigmod2014-pc/ ; python3 python/queries-all.py'
