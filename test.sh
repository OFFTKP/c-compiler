#!/bin/bash
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Test lexer
cd ${SCRIPT_DIR}/lexer/qa
cmake -B build &> /dev/null
cmake --build build &> /dev/null
cd build
ctest --output-on-failure