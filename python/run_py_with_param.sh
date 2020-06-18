#!/usr/bin/env bash

PARAMS="${@:4}"

python3 main.py --data_path $1 --queries_to_run $3 --mode with_param --query_args  ${PARAMS// /,}