# Find all dependencies
find_package(PkgConfig REQUIRED)
if(APPLE)
  pkg_check_modules(nlohmann_json REQUIRED IMPORTED_TARGET nlohmann_json)
else(APPLE)
  find_package(nlohmann_json 3.10.5 REQUIRED)
  set(NLOHMANN_TARGET nlohmann_json::nlohmann_json)
endif(APPLE)
# Find Python using modern CMake
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# Setup python version and path correctly
set(ENV{PYTHONPATH} ${CMAKE_SOURCE_DIR}:$ENV{PYTHONPATH})

# We want to define the PYTHON_EXECUTABLE which is used in our project scope
if (NOT DEFINED PYTHON_EXECUTABLE)
  # Check for the environment variable PYTHON
  if(NOT "$ENV{PYTHON}" STREQUAL "")
    set(PYTHON_EXECUTABLE $ENV{PYTHON})
    message(STATUS "Using ENV python: $ENV{PYTHON}")
  # Check for a python installation in our autoproj workspace
  elseif(NOT "$ENV{AUTOPROJ_CURRENT_ROOT}" STREQUAL "" AND EXISTS "$ENV{AUTOPROJ_CURRENT_ROOT}/install/bin/python")
    set(PYTHON_EXECUTABLE "$ENV{AUTOPROJ_CURRENT_ROOT}/install/bin/python")
    message(STATUS "Using autoproj python: $ENV{AUTOPROJ_CURRENT_ROOT}/install/bin/python")
  # Use the Python found by find_package if available
  elseif(Python3_FOUND)
    set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
    message(STATUS "Using Python found by find_package: ${Python3_EXECUTABLE}")
  else()
    # Finally, use the default system python version
    set(PYTHON_EXECUTABLE "python3")
    message(STATUS "Using default python.")
  endif()
endif()

# After selecting the python executable, identify the corresponding version strings
execute_process(
  COMMAND ${PYTHON_EXECUTABLE} -c "import sys; print(sys.version_info.major)"
  OUTPUT_VARIABLE PYTHON_VERSION_MAJOR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${PYTHON_EXECUTABLE} -c "import sys; print(sys.version_info.minor)"
  OUTPUT_VARIABLE PYTHON_VERSION_MINOR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Python: ${PYTHON_EXECUTABLE}")
message(STATUS "Python version: ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")

# Define PYTHON_SITELIB_INSTALL_DIR based on found Python version
if (NOT DEFINED PYTHON_SITELIB_INSTALL_DIR)
  set(PYTHON_SITELIB_INSTALL_DIR "lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages")
endif()

message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "PYTHON_EXECUTABLE: ${PYTHON_EXECUTABLE}")
message(STATUS "PYTHON_SITELIB_INSTALL_DIR: ${PYTHON_SITELIB_INSTALL_DIR}")

###########################
# PYBIND11 Python Binding #
###########################

find_package(pybind11 REQUIRED)
find_package(pybind11_json REQUIRED)

