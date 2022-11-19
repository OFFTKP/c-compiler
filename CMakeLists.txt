cmake_minimum_required(VERSION 3.2)
enable_testing()
project(Compiler)
set(RootPath ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory("verifier")
# Create the expected outcome files
file(GLOB LexerDataFiles "lexer/qa/data/compare/src/*.c")
set(VerifierCommands)
foreach(file ${LexerDataFiles})
    get_filename_component(filename ${file} NAME)
    set(out ${RootPath}/lexer/qa/data/compare/expected/${filename}.e)
    list(APPEND VerifierCommands
        COMMAND Verifier l ${file} ${out})
endforeach()

add_custom_target(
    RunVerifier ALL
    ${VerifierCommands}
)

add_executable(
    Compiler
    ${RootPath}/main.cxx
    ${RootPath}/lexer/lexer.cxx
    ${RootPath}/preprocessor/preprocessor.cxx
    ${RootPath}/dispatcher/dispatcher.cxx
    ${RootPath}/boolean_evaluator/boolean_evaluator.cxx
)
target_include_directories(Compiler PUBLIC ${RootPath}/)

project(TestPreprocessor)
add_executable(
    TestPreprocessor 
    ${RootPath}/common/qa/test_runner.cxx
    ${RootPath}/preprocessor/qa/test_preprocessor.cxx
    ${RootPath}/common/qa/test_base.cxx
    ${RootPath}/preprocessor/preprocessor.cxx
    ${RootPath}/boolean_evaluator/boolean_evaluator.cxx
)
target_link_libraries(TestPreprocessor cppunit)
target_compile_definitions(TestPreprocessor PRIVATE TEST_DATA_FILEPATH="${RootPath}/preprocessor/qa/data")
target_include_directories(TestPreprocessor PUBLIC ${RootPath}/)
add_test(NAME TestPreprocessor COMMAND TestPreprocessor)

project(TestLexer)
add_executable(
    TestLexer 
    ${RootPath}/common/qa/test_runner.cxx
    ${RootPath}/lexer/qa/test_lexer.cxx
    ${RootPath}/common/qa/test_base.cxx
    ${RootPath}/lexer/lexer.cxx
    ${RootPath}/preprocessor/preprocessor.cxx
    ${RootPath}/boolean_evaluator/boolean_evaluator.cxx
)
target_include_directories(TestLexer PUBLIC ${RootPath}/)
target_compile_definitions(TestLexer PRIVATE TEST_DATA_FILEPATH="${RootPath}/lexer/qa/data")
target_link_libraries(TestLexer cppunit)
add_test(NAME TestLexer COMMAND TestLexer)

project(TestBooleanEvaluator)
add_executable(
    TestBooleanEvaluator 
    ${RootPath}/common/qa/test_runner.cxx
    ${RootPath}/boolean_evaluator/qa/test_boolean_evaluator.cxx
    ${RootPath}/common/qa/test_base.cxx
    ${RootPath}/boolean_evaluator/boolean_evaluator.cxx
)
target_include_directories(TestBooleanEvaluator PUBLIC ${RootPath}/)
target_compile_definitions(TestBooleanEvaluator PRIVATE TEST_DATA_FILEPATH="${RootPath}/boolean_evaluator/qa/data")
target_link_libraries(TestBooleanEvaluator cppunit)
add_test(NAME TestBooleanEvaluator COMMAND TestBooleanEvaluator)
