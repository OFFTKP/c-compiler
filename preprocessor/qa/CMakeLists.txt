cmake_minimum_required(VERSION 3.2)
enable_testing()
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH ProcessorPath)
cmake_path(GET ProcessorPath PARENT_PATH RootPath)
project(TestPreprocessor)
add_executable(
    TestPreprocessor 
    ${RootPath}/common/qa/test_runner.cxx
    ${RootPath}/preprocessor/qa/test_preprocessor.cxx
    ${RootPath}/common/qa/test_base.cxx
    ${RootPath}/preprocessor/preprocessor.cxx
)
target_include_directories(TestPreprocessor PUBLIC ${RootPath}/)
target_compile_definitions(TestPreprocessor PRIVATE TEST_DATA_FILEPATH="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_link_libraries(TestPreprocessor cppunit)
add_test(NAME TestPreprocessor COMMAND TestPreprocessor)