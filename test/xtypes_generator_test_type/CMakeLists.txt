cmake_minimum_required(VERSION 3.10)
project(xtypes_generator_test_type VERSION 0.0.1 DESCRIPTION "Test XType for testing xtypes_generator")

find_package(xtypes_generator REQUIRED)
set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS True)
xtypes_project()

### If you provide your package information via cmake, feel free to use the following lines
# configure_file(${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-config.cmake @ONLY)
# install(FILES ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-config.cmake DESTINATION lib/cmake/${PROJECT_NAME})
