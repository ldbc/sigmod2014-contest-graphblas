# SIGMOD 2014 Programming Contest

See our website at <https://ftsrg.mit.bme.hu/paper-lsgda2020/>.

## Use [pygraphblas](https://github.com/michelp/pygraphblas) in notebooks

### Requirements
- Docker

### Steps
- Clone [fixes branch](https://github.com/szarnyasg/pygraphblas/tree/fixes) of https://github.com/szarnyasg/pygraphblas/
- Run `./build.sh v3.2.0` to build the Docker image with slight modifications
- Mount this repo to the container by adding `-v «PATH»/sigmod2014-pc:/home/jovyan/sigmod2014-pc` to [notebook.sh](https://github.com/szarnyasg/pygraphblas/blob/fixes/notebook.sh)
- Start `./notebook.sh`
- Open Jupyter Notebook at: http://127.0.0.1:8888/?token=«TOKEN»

## Data

### Preprocessing the provided data sets

Navigate to the `preprocess/` directory and use the `convert-csvs.sh` script with the files and their respective character encoding.

```bash
cd preprocess/
./convert-csvs.sh /path/to/outputDir-1k /path/to/outputDir-1k-converted macintosh
./convert-csvs.sh /path/to/outputDir-10k /path/to/outputDir-10k-converted
```

:bulb: It is possible to skip this step – data sets that **have already been converted** with this script are available in Gabor's Dropbox as zip archives:

* 1k persons: <https://www.dropbox.com/s/sgrwihjji551teq/sf1k.zip?dl=1>
* 10k persons: <https://www.dropbox.com/s/goabh7c3q5k4ex4/sf10k.zip?dl=1>

### Generating your own data sets

Use the [LDBC Datagen's `grb-exps` branch](https://github.com/ldbc/ldbc_snb_datagen/tree/grb-exps) to generate data sets. Navigate to the `ldbc_socialnet_dbgen` directory, edit the `numtotalUser` value and use the `./run.sh` script.

The generated data set will be in the `outputDir/` directory. The conversion script works on these files as well.

## Experiments with Neo4j.

See [instructions on how to experiment with Neo4j](neo4j.md).

