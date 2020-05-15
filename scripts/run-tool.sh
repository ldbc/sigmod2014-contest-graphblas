#!/bin/bash

set -e

TOOL=${1:-cpp/cmake-build-release/sigmod2014pc_cpp}
TOOL_NAME=${TOOL_NAME:-$(basename "$(dirname "$(dirname $TOOL)")")}

SIZE=${2:-o1k}
SIZE_ONLY_NUMBER=${SIZE//[^0-9]/}

CSVS_BASE_FOLDER=${3:-csvs}
PARAMS_BASE_FOLDER=${PARAMS_BASE_FOLDER:-params}
RESULTS_BASE_FOLDER=${RESULTS_BASE_FOLDER:-results}

QUERIES=${QUERIES:-"1,2,3,4"}
QUERIES="${QUERIES//,/ }"

DATE=$(date "+%Y-%m-%dT%H.%M.%S")

RESULTS_FOLDER="$RESULTS_BASE_FOLDER/$SIZE/$TOOL_NAME"
LOG_PATH="$RESULTS_FOLDER/$DATE.log"
RESULTS_PATH="$RESULTS_FOLDER/$DATE.csv"

mkdir -p "$RESULTS_FOLDER"

echo -e \
"Time: $DATE\n"\
"Starting $TOOL_NAME at $TOOL\n"\
"Size: $SIZE\n"\
"CSVs from: $CSVS_BASE_FOLDER\n"\
"Params from: $PARAMS_BASE_FOLDER\n"\
"Results go to $RESULTS_FOLDER\n"\
"Queries to run: $QUERIES\n"\
  | tee -a "$LOG_PATH"

for i in $QUERIES
do
  PARAMS_PATH="$PARAMS_BASE_FOLDER/$SIZE/query$i.csv"

  while IFS= read -r line
  do
    PARAMS="${line//,/ }"
    COMMAND=("$TOOL" "$CSVS_BASE_FOLDER/$SIZE/" PARAM $i $PARAMS)

    echo Run query$i: "${COMMAND[@]}" | tee -a "$LOG_PATH"
    # extend result lines
    echo -n $TOOL_NAME,$SIZE_ONLY_NUMBER, | tee -a "$LOG_PATH" "$RESULTS_PATH"

    # run tool and write results to CSV and log every output
    "${COMMAND[@]}" \
      > >(tee -a "$LOG_PATH" "$RESULTS_PATH") \
      2> >(tee -a "$LOG_PATH" 1>&2)

  done < "$PARAMS_PATH"
done
