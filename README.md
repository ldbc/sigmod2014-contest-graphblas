# SIGMOD 2014 Programming Contest

See our website at <https://ftsrg.mit.bme.hu/paper-hpec2020/>.

## Use [pygraphblas](https://github.com/michelp/pygraphblas) in notebooks

### Requirements
- Docker

### Steps
- Clone [fixes branch](https://github.com/szarnyasg/pygraphblas/tree/fixes) of https://github.com/szarnyasg/pygraphblas/
- Run `./build-custom.sh` to build the Docker image with slight modifications
- To run pygrpahblas in Docker:
    - Mount this repo to the container by adding `-v «PATH»/sigmod2014-pc:/home/jovyan/sigmod2014-pc` to [notebook.sh](https://github.com/szarnyasg/pygraphblas/blob/fixes/notebook.sh), where `«PATH»` is the absolute path to this repository.
    - Start `./notebook.sh`
    - Open Jupyter Notebook at: http://127.0.0.1:8888/?token=«TOKEN»
- To install pygraphblas on your host machine, navigate to the `pygraphblas` directory:
    - Install its dependencies: `sudo pip3 install -r notebook-requirements.txt`
    - Make sure it's uninstalled, then (re)install it with: `pip3 uninstall pygraphblas; python3 setup.py install --user`

## Data

### Preprocessing the provided data sets

Navigate to the `preprocess/` directory and use the `convert-csvs.sh` script with the files and their respective character encoding.

```bash
cd preprocess/
./convert-csvs.sh /path/to/outputDir-1k /path/to/outputDir-1k-converted macintosh
./convert-csvs.sh /path/to/outputDir-10k /path/to/outputDir-10k-converted
```

### Preprocessed data sets (`o{1,10}k`)

:bulb: It is possible to skip this step – data sets that **have already been converted** with this script are available in Gabor's Dropbox as ZIP archives.
This data is used with the original sample queries and answers as test suite.

* 1k persons:  <https://www.dropbox.com/s/sgrwihjji551teq/o1k.zip?dl=1>
* 10k persons: <https://www.dropbox.com/s/goabh7c3q5k4ex4/o10k.zip?dl=1>

### Generating your own data sets (`p{1,10,...}k`)

Use the [LDBC Datagen's `early2014` release](https://github.com/ldbc/ldbc_snb_datagen/releases/tag/early2014) to generate data sets. Navigate to the `ldbc_socialnet_dbgen` directory, edit the `numtotalUser` value and use the `./run.sh` script.

The generated data set will be in the `outputDir/` directory. The conversion script works on these files as well.

Generated datasets:

* 1k persons:    <https://www.dropbox.com/s/d68r4hddl28cli3/p1k.tar.zst?dl=1>
* 10k persons:   <https://www.dropbox.com/s/2xwhz1o7ioajy6x/p10k.tar.zst?dl=1>
* 100k persons:  <https://www.dropbox.com/s/ex4byfcs0af72gc/p100k.tar.zst?dl=1>
* 1000k persons: <https://www.dropbox.com/s/4viydt858yzl3cy/p1000k.tar.zst?dl=1>

## Generate query parameters

See [here](cpp/README.md#generate-new-query-parameters).

## Measurements

After downloading data and generating query parameters, build the tool to be measured (e.g. [C++ implementation](cpp/README.md)).

### Single tool
Run measurements using `scripts/run-tool.sh`. Running it without parameters is the same as the following. (More details [here](scripts/run-tool.sh).)

```bash
QUERIES=1,2,3,4 scripts/run-tool.sh cpp/cmake-build-release/sigmod2014pc_cpp o1k csvs
```

Log file and measurement results in CSV are saved to `results/$SIZE/$TOOL_NAME`. Look for the latest timestamp.

### Multiple tools

Run the following:

```bash
ITERATION_COUNT=1 QUERIES=1,2,3,4 scripts/run.sh "«TOOL1_PATH» «TOOL2_PATH»..." 1,10,100,1000 csvs
```

Parameters: paths of tools, sizes, CSVs folder.

## Experiments with Neo4j

See [instructions on how to experiment with Neo4j](neo4j.md).
