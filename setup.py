import os
import sys
from setuptools import setup, Extension

# Read the long description from README
try:
    with open("README.md", "r", encoding="utf-8") as fh:
        long_description = fh.read()
except FileNotFoundError:
    long_description = (
        "A fast native C recursive file scanner and analyzer "
        "with tree, JSON, CSV export and advanced analytics."
    )

# Detect platform-specific compiler optimization and linking flags
if sys.platform == "win32":
    # MSVC optimizations and strict warnings
    extra_compile_args = ['/O2', '/W4']
    extra_link_args = []
else:
    # POSIX optimizations (GCC/Clang)
    extra_compile_args = [
        '-O3',
        '-Wall',
        '-Wextra',
        '-Wno-unused-parameter',
    ]
    # -march=native emits instructions tuned to the CPU that runs the build.
    # That is ideal for a local source install, but a wheel compiled with it can
    # crash with SIGILL on a machine with a different CPU. It is therefore
    # opt-in: build with ANSCOM_NATIVE=1 when the build host and run host share
    # the same CPU (the common `pip install` source-build case).
    if os.environ.get("ANSCOM_NATIVE") == "1":
        extra_compile_args.insert(1, '-march=native')
    extra_link_args = ['-pthread']

module = Extension(
    'anscom',
    sources=['anscom.c'],
    extra_compile_args=extra_compile_args,
    extra_link_args=extra_link_args,
)

setup(
    name='anscom',
    version='1.5.2',
    author='Aditya Narayan Singh',
    author_email='adityansdsdc@outlook.com',
    description=(
        'High-performance native C recursive file scanner: '
        'multi-threaded, terabyte-scale, with CSV/JSON/Tree export, '
        'duplicate detection, largest-N report, and regex filtering.'
    ),
    long_description=long_description,
    long_description_content_type="text/markdown",

    # Primary links shown on PyPI
    url='https://github.com/PC5518/anscom-nfie-python-extension',
    project_urls={
        "Homepage":    "https://anscomqs.github.io/anscom/",
        "Source":      "https://github.com/PC5518/anscom-nfie-python-extension",
        "Bug Tracker": "https://github.com/PC5518/anscom-nfie-python-extension/issues",
    },

    ext_modules=[module],

    # No hard dependencies — anscom core is pure C
    install_requires=[],

    # Optional dependencies
    extras_require={
        # pip install anscom[all]
        "all": [],
    },

    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Topic :: System :: Filesystems",
        "Topic :: System :: Systems Administration",
        "Topic :: Utilities",
        "Intended Audience :: Developers",
        "Intended Audience :: System Administrators",
        "Development Status :: 5 - Production/Stable",
    ],
    python_requires='>=3.6',

    keywords=[
        'filesystem', 'scanner', 'file-analysis', 'directory',
        'recursive', 'multithreaded', 'C-extension', 'duplicate-detection',
        'disk-usage', 'audit', 'enterprise', 'pwd', 'cwd', 'current-directory',
        'mascot'
    ],
)
