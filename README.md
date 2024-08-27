CircleCI build and test:


# X-Types Generator

This repository provides the XType base class and all necessary tools to generate your custom XTypes.

The xtypes_generator was initiated and is currently developed at the
[Robotics Innovation Center](http://robotik.dfki-bremen.de/en/startpage.html) of the
[German Research Center for Artificial Intelligence (DFKI)](http://www.dfki.de) in Bremen,
together with the [Robotics Group](http://www.informatik.uni-bremen.de/robotik/index_en.php)
of the [University of Bremen](http://www.uni-bremen.de/en.html).

## What are XTypes?
XTypes are an object-oriented type definition which like a normal class hold properties and can have member functions;
but furthermore, they can hold relations to other XTypes.
Due to their neat implementation, XTypes instances are out of the box ready to be stored in databases.

Moreover, they come directly with Python bindings, so you don't have to worry about binding everything yourself. It's already done for you.

The XTypes are defined as templates.
An exemplary template can be found [here](doc/template.yaml).
Please have a look in [Templates.md](doc/Templates.md) on how to write your own template.

## So in short how do I use them?
To use XTypes as the basis of your project you have to do the following:

1. Install this project [see Installation Instructions](#Installation)
2. Setup your project directory. You can do this using `xtypes_generator create_xtypes_project`.
3. Write the CMakeLists.txt file for your project. And add:
    ```cmake
    find_package(xtypes_generator REQUIRED)
    xtypes_project()
    ```
    Further info [here](#CMake)
  
4. Define your types using template files like [doc/template.yaml](doc/template.yaml)
5. Run cmake once in the build directory:
    ```bash
    cd $YOUR_PROJECT
    mkdir build
    cd build
    cmake ..
    ```
    Now all files are generated for you.

6. Copy the skeleton files for your XTypes from build/auto_generated_files/skeleton_files to your source directory, e.g.:
    ```bash
    cd $YOUR_PROJECT
    cp -rn build/auto_generated_files/skeleton_files/src/* src/
    ```
    If need to make adaptations you can copy the header skeleton files as well.
    You only need to have the files in your include/ src/ directories you want to override.

7. Fill those files with your source code.

8. Do a full build and install and you will have C++ Library and python module ready to go.
    ```bash
    cd $YOUR_PROJECT
    mkdir build
    cd build
    cmake ..
    make install
    ```

---
# Installation
## Dependencies
Please make sure you have the following dependencies installed
- CMake >= 3.10
- nlohmann_json >= 3.10.5
- pybind11 >=2.9.1
- pybind11_json >=0.2.12
- Python >=3.6 <4
- python-yaml

## Setup
### Manual
1. Clone this repository
2. Build and install via:
  ```bash
  cd xtypes_generator
  mkdir build
  cd build
  cmake ..
  make install
  ```

### Autoproj
1. Add this repository to your autoproj setup.
2. Then run in your autoproj workspace:
  ```bash
  aup xtypes_generator
  amake xtypes_generator
  ```

---

# Usage
## CMake
We define a new CMake macro called `xtypes_project` to customize the execution you can use the following arguments, but normally you should be fine using just `xtypes_project()`.

Flags:
  - `DEACTIVATE_PYTHON_BINDINGS`: If you don't want to build the Python bindings to your code

Arguments:
  - `SOURCE_DIRECTORY`:
    The directory which contains include and src dirs.<br>
    _Default_: `${CMAKE_CURRENT_SOURCE_DIR}`
  - `TEMPLATE_DIRECTORY`: The template directory.<br>
    _Default_: `${CMAKE_CURRENT_SOURCE_DIR}/templates/`
  - `PYBIND_DIRECTORY`: The directory from where to include any further custom pybind files.<br>
    _Default_: `${CMAKE_CURRENT_SOURCE_DIR}/pybind/`
  - `AUTO_GEN_DIRECTORY`: The directory where to put the generated files.<br>
    _Default_: `${CMAKE_BINARY_DIR}/auto_generated_files`
  - `SKELETON_DIRECTORY `: The directory where to write the skeleton files.<br>
    _Default_: `${AUTO_GEN_DIRECTORY}/skeleton_files/`
  - `COLLECTED_DIRECTORY`: The directory where to put the collected files.<br>
    _Default_: `${CMAKE_BINARY_DIR}/build_files`
  - `NAMESPACE`: The namespace to use. <br>
    _Default_: `${PROJECT_NAME}`
  - `PYTHON_EXECUTABLE`: The python executable to build python bindings for.<br>
    _Default_: The currently available python.


## Tools/Scripts
This project comes with several scripts/tools you can access via `xtypes_generator` command in your terminal.
See `xtypes_generator -h` and `xtypes_generator COMMAND -h` for more info on those tools.

### `create_xtypes_project`
This tool sets up a new directory and the parameters for your new XTypes project.
You can pass the necessary info either by the command line arguments or run it with the `-i` option to be asked interactively.

> ATTENTION: The now following tools are mainly used internally. Prefer to use the CMake macro unless you know what you are doing.

### `xtypes_generator`
Used for code generation from template yaml-files of the XTypes. Generates C++ base classes, user-customizable skeleton-files and the python bindings.
For usage details see: `xtypes_generator -h`

### `xtypes_registry_generator`
Generates C++ and pybind files for the Registry class.
For usage details see: `xtypes_registry_generator -h`

### `xtypes_files_.*`
These tools are mainly used internally and deliver file listing and management support when building xtype representations with the xtypes_generator.

For details see:
 - `xtypes_files_get -h`
 - `xtypes_files_copy -h`
 - `xtypes_files_get_and_copy -h`

# Documentation
To create the Doxygen documentation, simply do:
```bash
  cd xtypes_generator/build
  cmake ..
  make doc
```
