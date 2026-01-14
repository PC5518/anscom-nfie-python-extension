from setuptools import setup, Extension

# Read the long description from README
with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

module = Extension(
    'anscom',
    sources=['anscom.c'],
    # Optional: Compile arguments can be added here if needed
    # extra_compile_args = ['-O3'] 
)

setup(
    name='anscom',  # Check PyPI if this name is taken!
    version='0.6.0',
    author='Aditya Narayan Singh',
    author_email='adityansdsdc@outlook.com',
    description='A fast native C recursive file scanner and analyzer',
    long_description=long_description,
    long_description_content_type="text/markdown",
    url='https://github.com/PC5518', # Optional
    ext_modules=[module],
    classifiers=[
        "Programming Language :: Python :: 3",
        "Programming Language :: C",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires='>=3.6',
)