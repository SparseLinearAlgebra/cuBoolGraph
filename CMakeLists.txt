cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)

set(CUBOOL_GRAPH_LIB_NAME cuboolgraph)
project(${CUBOOL_GRAPH_LIB_NAME} LANGUAGES CXX)

add_library(${CUBOOL_GRAPH_LIB_NAME} SHARED "")

set(CUBOOL_COPY_TO_PY_PACKAGE OFF)
set(CUBOOL_BUILD_TESTS OFF)
add_subdirectory(deps/cuBool)

# cubool is a name of CMakeTarget cmake target
target_link_libraries(${CUBOOL_GRAPH_LIB_NAME} PUBLIC cubool)

target_include_directories(${CUBOOL_GRAPH_LIB_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(src)

option(CUBOOL_GRAPH_ENABLE_TESTING "Compile tests for algorithms" OFF)

if(CUBOOL_GRAPH_ENABLE_TESTING)
  add_subdirectory(deps/googletest)
  add_subdirectory(deps/fast_matrix_market)
  add_subdirectory(tests)
endif()

