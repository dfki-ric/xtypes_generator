# Find all dependencies
find_package(PkgConfig REQUIRED)
if(APPLE)
  pkg_check_modules(nlohmann_json REQUIRED IMPORTED_TARGET nlohmann_json)
else(APPLE)
  find_package(nlohmann_json 3.10.5 REQUIRED)
  set(NLOHMANN_TARGET nlohmann_json::nlohmann_json)
endif(APPLE)

# Find Python using modern CMake
find_package(Python REQUIRED COMPONENTS Interpreter Development)

# Check for Autoproj environment and use its Python if needed
if (NOT PYTHON_FOUND)
  if (NOT "$ENV{AUTOPROJ_CURRENT_ROOT}" STREQUAL "" AND EXISTS "$ENV{AUTOPROJ_CURRENT_ROOT}/install/bin/python")
    set(PYTHON_EXECUTABLE "$ENV{AUTOPROJ_CURRENT_ROOT}/install/bin/python")
    message(STATUS "Using Autoproj Python: $ENV{AUTOPROJ_CURRENT_ROOT}/install/bin/python")

    # Optionally, find Python using this executable
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

    # Ensure these variables are used to configure the Python package directories
    set(Python_EXECUTABLE ${PYTHON_EXECUTABLE})
    set(Python_VERSION_MAJOR ${PYTHON_VERSION_MAJOR})
    set(Python_VERSION_MINOR ${PYTHON_VERSION_MINOR})
  else()
    message(FATAL_ERROR "Python executable from Autoproj environment is not found or not configured properly.")
  endif()
endif()

# Set PYTHON_SITELIB_INSTALL_DIR based on found Python version
if (NOT DEFINED PYTHON_SITELIB_INSTALL_DIR)
  set(PYTHON_SITELIB_INSTALL_DIR "lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages")
endif()

# Print Python configuration
message(STATUS "Python executable: ${Python_EXECUTABLE}")
message(STATUS "Python version: ${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}")
message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "PYTHON_SITELIB_INSTALL_DIR: ${PYTHON_SITELIB_INSTALL_DIR}")

###########################
# PYBIND11 Python Binding #
###########################

find_package(pybind11 REQUIRED)
find_package(pybind11_json REQUIRED)