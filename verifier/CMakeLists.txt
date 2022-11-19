find_package(BISON)
find_package(FLEX)
project(Verifier)
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH RootPath)
BISON_TARGET(MyParser verifier.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c)
FLEX_TARGET(MyScanner verifier.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.c)
ADD_FLEX_BISON_DEPENDENCY(MyScanner MyParser)
add_executable(Verifier
    main.c
    ${BISON_MyParser_OUTPUTS}
    ${FLEX_MyScanner_OUTPUTS}
)
target_include_directories(Verifier PUBLIC ${RootPath}/)
