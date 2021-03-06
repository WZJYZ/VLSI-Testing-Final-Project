#!/bin/bash

EVAL=./eval/golden_tdfsim
# EVAL=./eval/golden_tdfsim_faults
# EVAL=./eval/atpg

CIR_PATH=./sample_circuits
PAT_PATH=./tdf_patterns

CKT_FILE=$CIR_PATH/c$1.ckt
PAT_FILE=$PAT_PATH/c$1.pat

print_help () {
    echo "[Help]                          "
    echo "Usage:                          "
    echo "         ./eval.sh <ckt#> [#det]"
    echo "Example:                        "
    echo "         ./eval.sh 17           "
    echo "         ./eval.sh 17 8         "
}


if [ $# -eq 2 ]; then
    $EVAL -ndet $2 -tdfsim $PAT_FILE $CKT_FILE
elif [ $# -eq 1 ]; then
    $EVAL -ndet 1 -tdfsim $PAT_FILE $CKT_FILE
else
    print_help
fi
