#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Test preprocessor
# (cd ${SCRIPT_DIR}/preprocessor/qa &&
# cmake -B build &&
# cmake --build build &&
# cd build &&
# ctest --output-on-failure)

# Test lexer
# (cd ${SCRIPT_DIR}/lexer/qa &&
# cmake -B build &&
# cmake --build build &&
# cd build &&
# ctest --output-on-failure)

# Test boolean evaluator
(cd ${SCRIPT_DIR}/boolean_evaluator/qa &&
cmake -B build &&
cmake --build build &&
cd build &&
ctest --output-on-failure)