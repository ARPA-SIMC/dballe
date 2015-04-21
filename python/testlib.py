# coding: utf-8
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import shlex
import os
import sys
import unittest

def main(testname):
    args = os.environ.get("ARGS", None)
    if args is None:
        return unittest.main()

    args = shlex.split(args);
    if args[0] != testname:
        return 0

    argv = [sys.argv[0]] + args[1:]
    unittest.main(argv=argv)
