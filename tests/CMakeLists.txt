set(TESTS_TARGET cuboolgraph_tests)

add_executable(${TESTS_TARGET} "main.cpp")

target_link_libraries(${TESTS_TARGET} PUBLIC ${CUBOOL_GRAPH_LIB_NAME})
target_link_libraries(${TESTS_TARGET} PUBLIC gtest)
target_link_libraries(${TESTS_TARGET} PUBLIC fast_matrix_market)

target_compile_definitions(${TESTS_TARGET} PUBLIC
                           TESTS_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/test_data/")

# tests files
target_sources(${TESTS_TARGET} PUBLIC "rpq_tests.cpp")
