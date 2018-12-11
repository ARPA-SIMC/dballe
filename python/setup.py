from setuptools import Extension, setup, command
import subprocess


def pkg_config_flags(options):
    return [
        s for s in subprocess.check_output(['pkg-config'] + options + ['libdballe']).decode().strip().split(" ") if s
    ]


dballe_module = Extension(
    '_dballe',
    sources=[
        "common.cc", "cursor.cc", "dballe.cc", "db.cc",
    ],
    language="c++",
    extra_compile_args=pkg_config_flags(["--cflags"]) + ["-std=c++11"],
    extra_link_args=pkg_config_flags(["--libs"]),
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
    install_requires=['wreport'],
)
