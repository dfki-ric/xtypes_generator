# Find all dependencies
find_package(PkgConfig REQUIRED)
if(APPLE)
  pkg_check_modules(nlohmann_json REQUIRED IMPORTED_TARGET nlohmann_json)
else(APPLE)
  find_package(nlohmann_json 3.10.5 REQUIRED)
  set(NLOHMANN_TARGET nlohmann_json::nlohmann_json)
endif(APPLE)

# Setup python version and path correctly
set(ENV{PYTHONPATH} ${CMAKE_SOURCE_DIR}:$ENV{PYTHONPATH})

# We want to define the PYTHON_EXECUTABLE which is used in our project scope

# Either we have the executable defined as cmake parameter
if (NOT DEFINED ${PYTHON_EXECUTABLE})
  # or we check for the environment variable PYTHON
  if(NOT "$ENV{PYTHON}" STREQUAL "")
    set(PYTHON_EXECUTABLE $ENV{PYTHON})
    message(STATUS "Using ENV python: $ENV{PYTHON}")
    # or for a python installation in our autoproj workspace
  elseif((NOT "$ENV{AUTOPROJ_CURRENT_ROOT}" STREQUAL "") AND (EXISTS "$ENV{AUTOPROJ_CURRENT_ROOT}/install/bin/python"))
    set(PYTHON_EXECUTABLE "$ENV{AUTOPROJ_CURRENT_ROOT}/install/bin/python")
    message(STATUS "Using autoproj python: $ENV{AUTOPROJ_CURRENT_ROOT}/install/bin/python")
  else()
    # or finally we use the default system python version
    set(PYTHON_EXECUTABLE "python")
    message(STATUS "Using default python.")
  endif()
endif()

# After selecting the python executable we identify the correspoding version strings
execute_process(
  COMMAND ${PYTHON_EXECUTABLE} -c "import sys;print(sys.version_info.major)"
  OUTPUT_VARIABLE PYTHON_VERSION_MAJOR
)
execute_process(
  COMMAND ${PYTHON_EXECUTABLE} -c "import sys;print(sys.version_info.minor)"
  OUTPUT_VARIABLE PYTHON_VERSION_MINOR
)
string(STRIP ${PYTHON_VERSION_MAJOR} PYTHON_VERSION_MAJOR)
string(STRIP ${PYTHON_VERSION_MINOR} PYTHON_VERSION_MINOR)
message(STATUS "Python: ${PYTHON_EXECUTABLE}")
message(STATUS "Python version: ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")


# PYTHON_SITELIB_INSTALL_DIR defines were to install our python site-packages
if (NOT DEFINED ${PYTHON_SITELIB_INSTALL_DIR})
  #set(PYTHON_SITELIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages")
  set(PYTHON_SITELIB_INSTALL_DIR "lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages")
endif()

message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "PYTHON_EXECUTABLE: ${PYTHON_EXECUTABLE}")
message(STATUS "PYTHON_SITELIB_INSTALL_DIR: ${PYTHON_SITELIB_INSTALL_DIR}")


###########################
# PYBIND11 Python Binding #
###########################

find_package(PythonLibs ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} REQUIRED)
find_package(pybind11 REQUIRED)
find_package(pybind11_json REQUIRED)
