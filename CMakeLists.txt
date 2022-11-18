cmake_minimum_required(VERSION 3.2)
project(Compiler)
set(RootPath ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(
    Compiler
    ${RootPath}/main.cxx
    ${RootPath}/lexer/lexer.cxx
    ${RootPath}/preprocessor/preprocessor.cxx
    ${RootPath}/dispatcher/dispatcher.cxx
    ${RootPath}/boolean_evaluator/boolean_evaluator.cxx
)
target_include_directories(Compiler PUBLIC ${RootPath}/)
