## Process this file with automake to produce Makefile.in

doxygen_doc_sources = []

mksnippet = find_program('./mksnippet')

doxygen_doc_sources += custom_target('aliases',
    output: 'aliases.dox',
    input: '../dballe/core/aliases.gperf',
    command: [mksnippet, 'alias', 'dox', '@INPUT@'],
    capture: true,
)

doxygen_doc_sources += custom_target('ltypes',
    output: 'ltypes.dox',
    input: '../dballe/msg/ltypes.txt',
    command: [mksnippet, 'levels', 'dox', '@INPUT@'],
    capture: true,
)

doxygen_doc_sources += custom_target('tranges',
    output: 'tranges.dox',
    command: [mksnippet, 'tranges', 'dox'],
    capture: true,
)

sphinx_doc_sources = [
    'conf.py',
    'index.rst',
    'howtos.rst',
    'reference.rst',
    'cpp_ref/index.rst',
    'cpp_ref/messages.rst',
    'cpp_ref/cursors.rst',
    'cpp_ref/db.rst',
    'fortran_how/index.rst',
    'fortran_how/errors.rst',
    'fortran_how/start.rst',
    'fortran_how/iorecords.rst',
    'fortran_how/query.rst',
    'fortran_how/insert.rst',
    'fortran_how/delete.rst',
    'fortran_how/attributes.rst',
    'fortran_how/describe.rst',
    'fortran_how/files.rst',
    'fortran_how/faq.rst',
    'fortran_ref/index.rst',
    'fortran_ref/concepts.rst',
    'fortran_ref/functions.rst',
    'fortran_ref/transactions.rst',
    'python_how/index.rst',
    'python_how/convertvars.rst',
    'python_how/dumpdb.rst',
    'python_how/explorer.rst',
    'python_how/insert.rst',
    'python_how/attributes.rst',
    'python_ref/index.rst',
    'python_ref/types.rst',
    'python_ref/messages.rst',
    'python_ref/cursors.rst',
    'python_ref/db.rst',
    'python_ref/explorer.rst',
    'python_ref/volnd.rst',
]

subdir('general_ref')


# Generate doxygen configuration
libdballe_dox = configure_file(output: 'libdballe.dox', input: 'libdballe.dox.in', configuration: {
    'VERSION': meson.project_version(),
    'top_srcdir': meson.source_root(),
    'builddir': meson.current_build_dir(),
    'DOXYGEN_OUTPUT_DIR': meson.current_build_dir(),
    'WREPORT_DOXYGEN_DIR': '.',
})

#
# Generate documentation
#
# Because of issues like https://github.com/mesonbuild/meson/issues/2320
# we cannot express the chain of what's going on here using meson.
# As a workaround, we build a script that runs doxygen and sphinx, and run it
# in a custom target.

build_docs = configure_file(output: 'build-docs', input: 'build-docs.in', configuration: {
    'doxygen': doxygen.path(),
    'sphinx_build': sphinx.path(),
    'doc_src': meson.current_source_dir(),
    'doc_build': meson.current_build_dir(),
    'top_builddir': meson.build_root(),
    'pythonpath_src': meson.source_root() / 'python',
    'pythonpath_build': meson.build_root() / 'python',
})

built_docs = custom_target('built_docs',
    input: [libdballe_dox, sphinx_doc_sources, doxygen_doc_sources],
    output: ['libdballe.doxytags', 'html'],
    depends: [libdballe, py_dballe],
    command: [build_docs],
    install_dir: docdir,
    install: true,
    console: true,
)
