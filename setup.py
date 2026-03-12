import sys
from setuptools import setup, Extension

# Read the long description from README
try:
    with open("README.md", "r", encoding="utf-8") as fh:
        long_description = fh.read()
except FileNotFoundError:
    long_description = "A fast native C recursive file scanner and analyzer"

# Detect platform-specific compiler optimization and linking flags
if sys.platform == "win32":
    # MSVC optimizations and strict warnings
    extra_compile_args =['/O2', '/W4']
    extra_link_args =[]
else:
    # Aggressive POSIX optimizations (GCC/Clang)
    extra_compile_args =['-O3', '-march=native', '-Wall', '-Wextra', '-Wno-unused-parameter']
    extra_link_args = ['-pthread']

module = Extension(
    'anscom',
    sources=['anscom.c'],
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
)

setup(
    name='anscom', 
    version='1.0.0',
    author='Aditya Narayan Singh',
    author_email='adityansdsdc@outlook.com',
    description='A fast native C recursive file scanner and analyzer',
    long_description=long_description,
    long_description_content_type="text/markdown",
    url='https://github.com/PC5518', 
    ext_modules=[module],
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',
)
