cmake_minimum_required(VERSION 3.2)
enable_testing()
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH BooleanEvaluatorPath)
cmake_path(GET BooleanEvaluatorPath PARENT_PATH RootPath)
project(TestBooleanEvaluator)
add_executable(
    TestBooleanEvaluator 
    ${RootPath}/common/qa/test_runner.cxx
    ${RootPath}/boolean_evaluator/qa/test_boolean_evaluator.cxx
    ${RootPath}/common/qa/test_base.cxx
    ${RootPath}/boolean_evaluator/boolean_evaluator.cxx
)
target_include_directories(TestBooleanEvaluator PUBLIC ${RootPath}/)
target_compile_definitions(TestBooleanEvaluator PRIVATE TEST_DATA_FILEPATH="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_link_libraries(TestBooleanEvaluator cppunit)
add_test(NAME TestBooleanEvaluator COMMAND TestBooleanEvaluator)