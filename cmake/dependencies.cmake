# Find all dependencies
find_package(PkgConfig REQUIRED)

if(APPLE)
  pkg_check_modules(nlohmann_json REQUIRED IMPORTED_TARGET nlohmann_json)
else()
  find_package(nlohmann_json 3.10.5 REQUIRED)
  set(NLOHMANN_TARGET nlohmann_json::nlohmann_json)
endif()

# Find Python 3 interpreter and development
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# Ensure Python was found before proceeding
if (Python3_FOUND)
  set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE} CACHE STRING "Python executable" FORCE)
  message(STATUS "Using detected Python executable: ${PYTHON_EXECUTABLE}")
else()
  message(FATAL_ERROR "Python3 not found! Make sure Python3 is installed and accessible in your system.")
endif()

# Set up Python path
set(ENV{PYTHONPATH} ${CMAKE_SOURCE_DIR}:$ENV{PYTHONPATH})

# Extract Python version 
execute_process(
  COMMAND ${PYTHON_EXECUTABLE} -c "import sys; print(sys.version_info.major, end='')"
  OUTPUT_VARIABLE PYTHON_VERSION_MAJOR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

execute_process(
  COMMAND ${PYTHON_EXECUTABLE} -c "import sys; print(sys.version_info.minor, end='')"
  OUTPUT_VARIABLE PYTHON_VERSION_MINOR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

if (NOT PYTHON_VERSION_MAJOR OR NOT PYTHON_VERSION_MINOR)
  message(FATAL_ERROR "Failed to detect Python version. Make sure ${PYTHON_EXECUTABLE} is valid and accessible.")
endif()

message(STATUS "Detected Python version: ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")

if (PYTHON_VERSION_MAJOR AND PYTHON_VERSION_MINOR)
  set(PYTHON_SITELIB_INSTALL_DIR "lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages")
else()
  message(WARNING " Python version detection failed! Defaulting to system Python site-packages.")
  set(PYTHON_SITELIB_INSTALL_DIR "lib/python3/site-packages")
endif()

message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "PYTHON_EXECUTABLE: ${PYTHON_EXECUTABLE}")
message(STATUS "PYTHON_SITELIB_INSTALL_DIR: ${PYTHON_SITELIB_INSTALL_DIR}")

###########################
# PYBIND11 Python Binding #
###########################

find_package(pybind11 REQUIRED)
find_package(pybind11_json REQUIRED)
