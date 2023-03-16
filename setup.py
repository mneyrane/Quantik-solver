from setuptools import setup, Extension
from Cython.Build import cythonize
from pathlib import Path

sourcedir= Path("quantik")
sourcefiles = ["pyquantik.pyx", "quantik.c"]
includes = ["quantik"]

extensions = [
    Extension(
        name="pyquantik", 
        sources=[str(sourcedir/src) for src in sourcefiles],
        extra_compile_args=["-O3"],
    )
]

setup(
    name='pyquantik',
    ext_modules=cythonize(extensions, include_path=includes),
    zip_safe=False,
)
