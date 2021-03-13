symbolic
========

[symbolic](https://github.com/tmigimatsu/symbolic) is a C++/Python library for
parsing and manipulating [Planning Domain Definition Language
(PDDL)](https://planning.wiki/_citedpapers/pddl1998.pdf) symbols for AI
planning. This library is built upon [VAL](https://github.com/KCL-Planning/VAL),
a library for validating PDDL specifications.

Installation
------------

This library is written in C++ with bindings generated for Python. Compiling
requires CMake 3.11 and C++17 (gcc >= 7). It will compile out of the box for
Ubuntu 20.04, but older systems require updating:

### Ubuntu 16.04
```
# Install gcc 7.
sudo apt-get update && sudo apt-get install -y software-properties-common
sudo apt-add-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update && sudo apt-get -y install build-essential gcc-7 g++-7
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 5
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 7
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 5
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 7

# Install latest CMake.
sudo apt-get update && sudo apt-get install -y apt-transport-https
ca-certificates gnupg wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null |
gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ xenial main'
sudo apt-get update && sudo apt-get install -y cmake kitware-archive-keyring
sudo rm /etc/apt/trusted.gpg.d/kitware.gpg
```

Python
------

.. autosummary::
   :toctree: python
   :template: custom_module.rst

   symbolic

C++
---

.. list-table::
   :widths: 10 90

   * - .. raw:: html

          <a class="reference external" href="cpp/namespacesymbolic.html"><code><span class="pre">symbolic</span></code></a>

     - symbolic C++ API

Indices and tables
------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
