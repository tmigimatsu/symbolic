[![Build Status](https://travis-ci.com/tmigimatsu/symbolic.svg?branch=master)](https://travis-ci.com/tmigimatsu/symbolic)

# symbolic

`symbolic` is a C++/Python library for parsing and manipulating [Planning Domain
Definition Language (PDDL)](https://planning.wiki/_citedpapers/pddl1998.pdf)
symbols for AI planning. This library is built upon
[VAL](https://github.com/KCL-Planning/VAL), a C++ library for validating PDDL
specifications.

See the documentation for `symbolic`
[here](https://tmigimatsu.github.io/symdb/).

## Quick Install

The quickest way to install `symbolic` is through pip.

### Python only

Use this to install `symbolic` in your virtual environment.
```sh
git clone https://github.com/tmigimatsu/symbolic.git
cd symbolic
pip install cmake
pip install .
```

### C++ and Python

This will run the appropriate CMake command to build `symbolic` locally in the `symbolic/build` folder.
```sh
git clone https://github.com/tmigimatsu/symbolic.git
cd symbolic
pip install cmake
pip install -e .
```

## Installation Requirements

This library is written in C++ with Python bindings automatically generated with
[pybind11](https://github.com/pybind/pybind11). Compiling requires `cmake >=
3.11` and support for C++17 (`gcc >= 7`). It will compile out of the box for
Ubuntu 20.04, but older systems require updating:

### Ubuntu 16.04

```sh
# Install gcc 7.
sudo apt-get update && sudo apt-get install -y software-properties-common
sudo apt-add-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update && sudo apt-get -y install build-essential gcc-7 g++-7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 5
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 7
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 5
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 7

# Install the latest CMake.
sudo apt-get update && sudo apt-get install -y apt-transport-https ca-certificates gnupg wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ xenial main'
sudo apt-get update && sudo apt-get install -y cmake kitware-archive-keyring
sudo rm /etc/apt/trusted.gpg.d/kitware.gpg
```

### Ubuntu 18.04

```sh
# Install the latest CMake.
sudo apt-get update && sudo apt-get install -y apt-transport-https ca-certificates gnupg wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ bionic main'
sudo apt-get update && sudo apt-get install -y cmake kitware-archive-keyring
sudo rm /etc/apt/trusted.gpg.d/kitware.gpg
```

## Installation

### C++ only
To compile `symbolic`, run from the project root directory:
```sh
mkdir build
cd build
cmake ..
make
```

If CMake attempts to use an incorrect version of Python, specify the correct
version manually:
```sh
cmake .. -DPYBIND11_PYTHON_VERSION=3.8
```

By default, `symbolic` will compile with examples from `apps/`. To turn this
off, run the cmake command with a different flag:
```sh
cmake .. -DBUILD_EXAMPLES=OFF
```

### Python Library

To install the Python library (after building the C++ binaries above), activate
your virtual environment and navigate to the `symbolic` root directory. Then
run:
```sh
pip install -e .
```

You should now be able to `import symbolic` in your virtual environment.
