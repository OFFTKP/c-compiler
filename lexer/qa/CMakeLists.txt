cmake_minimum_required(VERSION 3.2)
enable_testing()
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH LexerPath)
cmake_path(GET LexerPath PARENT_PATH RootPath)
project(TestLexer)
add_executable(
    TestLexer 
    ${RootPath}/common/test_runner.cxx
    ${RootPath}/lexer/qa/test_lexer.cxx
    ${RootPath}/common/test_base.cxx
    ${RootPath}/lexer/lexer.cxx
    ${RootPath}/preprocessor/preprocessor.cxx
)
target_include_directories(TestLexer PUBLIC ${RootPath}/)
target_compile_definitions(TestLexer PRIVATE TEST_DATA_FILEPATH="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_link_libraries(TestLexer cppunit)
add_test(NAME TestLexer COMMAND TestLexer)