#!/bin/bash
set -e # Abort with error if any command returns with something other than zero

############
# Install script for environment dependencies
# Run "$> sudo ./install_dependencies.sh" to install system-wide
# Run "$> ./install_dependencies.sh <path_to_prefix>" to install to a local prefix
############

# Build function for cloning and building a project from a repository
function build {
  local repository_url=$1
  local branch=$2
  local tag=$3
  local directory_name=$4
  local cmake_args=$5

  if [ -d "$directory_name" ]; then
    echo "Directory $directory_name already exists."
    read -r -p "Do you want to delete and re-download $directory_name? [y/N] " response
    if [[ "$response" =~ ^([yY][eE][sS]|[yY])+$ ]]; then
      rm -rf "$directory_name"
    else
      echo "Aborting!"
      exit -1
    fi
  fi

  echo "Cloning from $repository_url..."
  git clone --branch "$branch" --single-branch --depth 1 "$repository_url" "$directory_name" || { echo "Failed to clone repository."; exit 1; }
  cd "$directory_name"
  if [ "$tag" != "" ]; then
    git fetch --tags || { echo "Failed to fetch tags."; exit 1; }
    git checkout "tags/$tag" || { echo "Failed to checkout tag."; exit 1; }
  fi
  cd ..

  mkdir "$directory_name/build"
  cd "$directory_name/build"
  cmake .. $cmake_args || { echo "CMake configuration failed"; exit 1; }
  make -j install || { echo "Make install failed"; exit 1; }
  cd ../..
}


PREFIX=""
ABS_PREFIX=""
if [ "$#" -eq 1 ]; then
  ABS_PREFIX=$(readlink -f "$1")
  PREFIX="-DCMAKE_INSTALL_PREFIX=$ABS_PREFIX"

  echo "" > env.sh 
  echo "export CMAKE_PREFIX_PATH=$ABS_PREFIX" >> env.sh
  echo "export PKG_CONFIG_PATH=$ABS_PREFIX/lib/pkgconfig:$ABS_PREFIX/share/pkgconfig:$ABS_PREFIX/lib64/pkgconfig:\$PKG_CONFIG_PATH" >> env.sh
  echo "export LD_LIBRARY_PATH=$ABS_PREFIX/lib:$ABS_PREFIX/lib64:\$LD_LIBRARY_PATH" >> env.sh
  echo "export PATH=$ABS_PREFIX/bin:\$PATH" >> env.sh

  source env.sh
fi


build "https://github.com/nlohmann/json.git" "develop" "v3.10.5" "json_new" "$PREFIX"
build "https://github.com/pybind/pybind11.git" "stable" "" "pybind11" "$PREFIX"
build "https://github.com/pybind/pybind11_json.git" "master" "0.2.13" "pybind11_json" "$PREFIX"
