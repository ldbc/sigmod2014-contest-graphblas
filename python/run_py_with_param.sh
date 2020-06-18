#!/usr/bin/env bash

set -e

DATA_PATH=$(realpath "$1")/

# https://stackoverflow.com/a/9057392
# https://github.com/koalaman/shellcheck/wiki/SC2124#correct-code
PARAMS=( "${@:4}" )

PYTHON_DIR=$(dirname "$0")

# run Python script from python/ folder
cd "$PYTHON_DIR"

python3 main.py --data_path "$DATA_PATH" --queries_to_run $3 --mode with_param --query_args "${PARAMS// /,}"
