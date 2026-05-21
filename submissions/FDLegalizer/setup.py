# setup.py
from setuptools import setup, Extension
import pybind11

ext_modules = [
    Extension(
        "macro_packer",
        ["src/packer.cpp", "src/bindings.cpp"],
        include_dirs=[pybind11.get_include()],
        language="c++",
        extra_compile_args=["-O3", "-std=c++17"],
    ),
]

setup(
    name="macro_packer",
    ext_modules=ext_modules,
)