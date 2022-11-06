#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Test lexer
(cd ${SCRIPT_DIR}/lexer/qa &&
cmake -B build &&
cmake --build build &&
cd build &&
ctest --output-on-failure)