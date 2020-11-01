#!/usr/bin/env bash

set -e

# run Python script from python/ folder
PYTHON_DIR=$(dirname "$0")/..
cd "$PYTHON_DIR"

python3 main.py --mode from_file $*
