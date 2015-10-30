from setuptools import Extension, setup, command


dballe_module = Extension(
    'dballe',
    sources=[
        "common.cc", "cursor.cc", "dballe.cc", "db.cc", "record.cc",
    ],
    language="c++",
    extra_compile_args=['-std=c++11'],
)

setup(
    name="dballe",
    version="7.6.0",
    author="Enrico Zini",
    author_email="enrico@enricozini.org",
    maintainer="Emanuele Di Giacomo",
    maintainer_email="edigiacomo@arpa.emr.it",
    description="Fast on-disk database for meteorological data",
    long_description="Fast on-disk database for meteorological data",
    url="http://github.com/arpa-simc/dballe",
    license="GPLv2+",
    py_modules=[],
    packages=['dballe'],
    data_files=[],
    zip_safe=False,
    include_package_data=True,
    exclude_package_data={},
    ext_modules=[dballe_module],
)
