import setuptools
import os
import shutil

__project_name__ = "xtypes_generator"
__project_description__ = "This package is a parser for the YAML-represented XTypes and generates there C++ base files as well as the respective pybind11-code."
__project_version__ = "0.0.1"


if __name__ == "__main__":
    # install python package/script
    setuptools.setup(
        name=__project_name__,  # Replace with your own username
        version=__project_version__,
        author="Moritz Schilling, Priyanka Chodhury, Henning Wiedemann, Malte Langosz",
        author_email="henning.wiedemann@dfki.de",
        description=__project_description__,
        url="https://git.hb.dfki.de/modkom/xtypes_generator",
        packages=setuptools.find_packages(),
        include_package_data=True,
        package_data={'': ["data/"+f for f in os.listdir("/home/dfki.uni-bremen.de/pchowdhury/modkom_sw_backbone/representation/xtypes_generator/xtypes_generator/data")], },
        classifiers=[
            "Programming Language :: Python :: 3",
            "Programming Language :: Python :: 3.6",
            "Programming Language :: Python :: 3.8",
            "Programming Language :: Python :: 3.10",
            "Operating System :: OS Independent",
        ],
        python_requires='>=3.6',
        # install_requires=[
        #     "pyyaml",
        # ],
        entry_points={
            'console_scripts': [
                'xtypes_generator=xtypes_generator.xtypesgen:main'
            ]
        },
    )
