
cmake_minimum_required(VERSION 3.2)
enable_testing()

function(join VALUES GLUE OUTPUT)
  string (REGEX REPLACE "([^\\]|^);" "\\1${GLUE}" _TMP_STR "${VALUES}")
  string (REGEX REPLACE "[\\](.)" "\\1" _TMP_STR "${_TMP_STR}") #fixes escaping
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH LexerPath)
cmake_path(GET LexerPath PARENT_PATH RootPath)

include_directories(${RootPath}/)
include_directories(${RootPath}/cppunit/src/src/)
file(GLOB DATA_FILENAMES "data/*.c")
list(LENGTH DATA_FILENAMES DATA_FILENAMES_LEN)
if (DATA_FILENAMES_LEN MATCHES 0)
    message( FATAL_ERROR "You can not do this at all, CMake will exit." )
endif()
foreach(item ${DATA_FILENAMES})
    list(APPEND DATA_FILES "\"${item}\",")
endforeach()

project(TestLexer)
add_executable(TestLexer ${RootPath}/common/test_runner.cxx test_lexer.cxx ${RootPath}/lexer/lexer.cxx ${RootPath}/preprocessor/preprocessor.cxx)
target_compile_definitions(TestLexer PRIVATE DATA_FILES=${DATA_FILES})
target_link_directories(TestLexer PRIVATE ${RootPath}/cppunit)
target_link_libraries(TestLexer cppunit)
add_test(NAME TestLexer COMMAND TestLexer)