cmake_minimum_required(VERSION 3.10)

project(xtypes_generator
  VERSION 0.0.1
  DESCRIPTION "This package is a parser for the YAML-represented XTypes and generates there C++ base files as well as the respective pybind11-code."
)

include(${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake)

# Create and install the version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(${CMAKE_BINARY_DIR}/cmake/xtypes_generator-config-version.cmake
  VERSION ${xtypes_generator_VERSION}
  COMPATIBILITY AnyNewerVersion
)
# include our cmake tool to use it
configure_file(
   ${CMAKE_SOURCE_DIR}/cmake/Tools.cmake.in
   ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}Tools.cmake @ONLY)

include(${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}Tools.cmake)

set(TOOLS_LINE "include(\"\${_prefix}/lib/cmake/${PROJECT_NAME}/${PROJECT_NAME}Tools.cmake\")")
# use it to generate pybind
# 20220713 MS: We cannot change the namespace to xtypes here, because then the python bindings will fail
xtypes_project(USE_LOCAL)

# first install the tool to use it afterwards
configure_file(
  ${CMAKE_SOURCE_DIR}/setup.py.in
  ${CMAKE_SOURCE_DIR}/setup.py @ONLY
)

add_custom_target(python_tools ALL
  COMMAND ${PYTHON_EXECUTABLE} -m pip install --upgrade ${CMAKE_SOURCE_DIR} --target ${CMAKE_BINARY_DIR}/python_tools
  # COMMAND cp -r ${CMAKE_BINARY_DIR}/python_tools/xtypes_generator* ${CMAKE_INSTALL_PREFIX}/${PYTHON_SITELIB_INSTALL_DIR}
)

install(DIRECTORY ${CMAKE_BINARY_DIR}/python_tools/ DESTINATION ${PYTHON_SITELIB_INSTALL_DIR} FILES_MATCHING REGEX "xtypes_generator.*/.*" )
install(PROGRAMS ${CMAKE_BINARY_DIR}/python_tools/bin/xtypes_generator DESTINATION bin)

configure_file(
  ${CMAKE_SOURCE_DIR}/cmake/dependencies.cmake
  ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-dependencies.cmake COPYONLY
)

install(FILES ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}Tools.cmake
        DESTINATION lib/cmake/${PROJECT_NAME}
)

install(FILES ${XTYPES_AUTO_GEN_DIRECTORY}/cmake/${PROJECT_NAME}-config.cmake
              ${XTYPES_AUTO_GEN_DIRECTORY}/cmake/${PROJECT_NAME}-config-version.cmake
              ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-dependencies.cmake
        DESTINATION lib/cmake/${PROJECT_NAME}
)

configure_file(
  ${CMAKE_SOURCE_DIR}/cmake/config.cmake.in
  ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-config.cmake @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-config.cmake
              ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}-config-version.cmake
        DESTINATION lib/cmake/${PROJECT_NAME}
)

install(FILES ${CMAKE_SOURCE_DIR}/cmake/config.cmake.in
        DESTINATION share/${PROJECT_NAME}/cmake
)

# documentation
configure_file(${CMAKE_SOURCE_DIR}/doc/Doxyfile.in ${CMAKE_BINARY_DIR}/doc/Doxyfile @ONLY)
add_custom_target(doc
  COMMAND doxygen ${CMAKE_BINARY_DIR}/doc/Doxyfile
)


#########
# Tests #
#########
# Because the tests need an already installed xtypes_generator, we have to manually trigger it
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test EXCLUDE_FROM_ALL)
