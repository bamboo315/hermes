project(test-refinements)

add_executable(${PROJECT_NAME} main.cpp)
include (${hermes2d_SOURCE_DIR}/CMake.common)
set_common_target_properties(${PROJECT_NAME})
set(BIN ${PROJECT_BINARY_DIR}/${PROJECT_NAME})
add_test(test-refinements-1 "${BIN}" domain.mesh)
add_test(test-refinements-2 "${BIN}" bracket.mesh)
add_test(test-refinements-3 "${BIN}" square.mesh)
add_test(test-refinements-4 "${BIN}" square_tri.mesh)
