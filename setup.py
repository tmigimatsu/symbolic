import pathlib
import re
import shutil
import subprocess
import sys

from packaging import version
import setuptools  # type: ignore
from setuptools.command import build_ext  # type: ignore


__version__ = "1.0.0"


class CMakeExtension(setuptools.Extension):
    def __init__(self, name: str):
        setuptools.Extension.__init__(self, name=name, sources=[])


class CMakeBuild(build_ext.build_ext):
    def run(self) -> None:

        if not self.inplace:
            try:
                out = subprocess.check_output(["cmake", "--version"])
            except OSError:
                raise RuntimeError(
                    "CMake must be installed to build the following extensions: "
                    + ", ".join(e.name for e in self.extensions)
                )

            cmake_version = version.Version(
                re.search(r"version\s*([\d.]+)", out.decode()).group(1)
            )
            if cmake_version < version.Version("3.13.0"):
                raise RuntimeError(
                    "CMake >= 3.13.0 is required. Install the latest CMake with 'pip install cmake'."
                )

        cmake_extensions = [e for e in self.extensions if isinstance(e, CMakeExtension)]
        for extension in cmake_extensions:
            self.build_extension(extension)

    def build_extension(self, extension: CMakeExtension) -> None:
        extension_dir = pathlib.Path(
            self.get_ext_fullpath(extension.name)
        ).parent.absolute()

        # Clean old build.
        for old_build in extension_dir.glob(
            "*.dylib" if sys.platform == "darwin" else "*.so"
        ):
            old_build.unlink()

        # Create new build folder.
        if self.inplace:
            build_dir = (pathlib.Path(__file__).parent / "build").absolute()
        else:
            build_dir = pathlib.Path(self.build_temp)
        build_dir.mkdir(parents=True, exist_ok=True)

        # Run CMake.
        build_type = "Debug" if self.debug else "Release"
        python_version = ".".join(map(str, sys.version_info[:3]))
        cmake_command = [
            "cmake",
            "-B" + str(build_dir),
            "-DBUILD_TESTING=OFF",
            "-DBUILD_EXAMPLES=OFF",
            "-DBUILD_PYTHON=ON",
            "-DPYBIND11_PYTHON_VERSION=" + python_version,
            "-DCMAKE_BUILD_TYPE=" + build_type,
        ]
        if not self.inplace:
            # Use relative paths for install rpath.
            rpath_origin = "@loader_path" if sys.platform == "darwin" else "$ORIGIN"
            cmake_command += [
                f"-DCMAKE_INSTALL_PREFIX={extension_dir}",
                f"-DCMAKE_INSTALL_RPATH={rpath_origin}",
            ]
        print(*cmake_command)
        self.spawn(cmake_command)

        # Build and install.
        make_command = ["cmake", "--build", str(build_dir)]
        if not self.inplace:
            make_command += ["--target", "install"]

        ncpus = (
            subprocess.check_output(["./ncpu.sh"], cwd="cmake").strip().decode("utf-8")
        )
        make_command += ["--", "-j" + ncpus]

        print(*make_command)
        self.spawn(make_command)

        if not self.inplace:
            # Copy pybind11 library.
            for file in (build_dir / "src" / "python").iterdir():
                if re.match(r".*\.(?:so|dylib)\.?", file.name) is not None:
                    shutil.move(str(file), str(extension_dir))

            # Copy C++ libraries.
            libdir = next(iter(extension_dir.glob("lib*")))
            for file in libdir.iterdir():
                if re.match(r".*\.(?:so|dylib)\.?", file.name) is not None:
                    shutil.move(str(file), str(extension_dir))


setuptools.setup(
    packages=["symbolic"],
    ext_modules=[CMakeExtension("symbolic.pysymbolic")],
    cmdclass={
        "build_ext": CMakeBuild,
    },
)
