import setuptools
import subprocess

from setuptools.command import build_ext


class CMakeExtension(setuptools.Extension):
    def __init__(self, name):
        setuptools.Extension.__init__(self, name, sources=[])


class CMakeBuild(build_ext.build_ext):
    def run(self):
        import distutils
        import re

        try:
            out = subprocess.check_output(["cmake", "--version"])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: "
                + ", ".join(e.name for e in self.extensions)
            )

        cmake_version = distutils.version.LooseVersion(
            re.search(r"version\s*([\d.]+)", out.decode()).group(1)
        )
        if cmake_version < "3.13.0":
            raise RuntimeError(
                "CMake >= 3.13.0 is required. Install the latest CMake with 'pip install cmake'."
            )

        for extension in self.extensions:
            self.build_extension(extension)

    def build_extension(self, extension: setuptools.Extension):
        import os
        import pathlib
        import shutil
        import sys

        # Skip CMake if building in place (pip install -e .).
        if self.inplace:
            return

        build_dir = pathlib.Path(self.build_temp)
        extension_dir = pathlib.Path(self.get_ext_fullpath(extension.name)).parent
        symbolic_dir = str(extension_dir / "symbolic")

        build_dir.mkdir(parents=True, exist_ok=True)
        extension_dir.mkdir(parents=True, exist_ok=True)

        # Run CMake.
        build_type = "Debug" if self.debug else "Release"
        python_version = ".".join(map(str, sys.version_info[:3]))

        # Use relative paths for install rpath.
        rpath_origin = "@loader_path" if sys.platform == "darwin" else "$ORIGIN"
        self.spawn(
            [
                "cmake",
                "-B" + self.build_temp,
                "-DBUILD_TESTING=OFF",
                "-DPYBIND11_PYTHON_VERSION=" + python_version,
                "-DCMAKE_BUILD_TYPE=" + build_type,
                "-DCMAKE_INSTALL_PREFIX=install",
                "-DCMAKE_INSTALL_RPATH=" + rpath_origin,
            ]
        )

        # Build and install.
        ncpus = (
            subprocess.check_output(["./ncpu.sh"], cwd="cmake").strip().decode("utf-8")
        )
        self.spawn(
            [
                "cmake",
                "--build",
                self.build_temp,
                "--target",
                "install",
                "--",
                "-j" + ncpus,
            ]
        )

        # Copy pybind11 library.
        for file in os.listdir(build_dir / "src" / "python"):
            if os.path.splitext(file)[1] in (".so", ".dylib"):
                file = str(build_dir / "src" / "python" / file)
                shutil.move(file, symbolic_dir)

        # Copy C++ libraries.
        for file in os.listdir(os.path.join("install", "lib")):
            if os.path.splitext(file)[1] in (".so", ".dylib"):
                file = os.path.join("install", "lib", file)
                shutil.move(file, symbolic_dir)


setuptools.setup(
    name="symbolic",
    version="0.1",
    author="Toki Migimatsu",
    author_email="takatoki@cs.stanford.edu",
    description="PDDL symbolic library",
    url="https://github.com/tmigimatsu/symbolic",
    packages=["symbolic"],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.6",
    ext_modules=[CMakeExtension("symbolic")],
    cmdclass={
        "build_ext": CMakeBuild,
    },
)
