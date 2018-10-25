import dballe
import io
import unittest
import os


def test_pathname(fname):
    if fname.startswith("."):
        return fname

    envdir = os.environ.get("DBA_TESTDATA", ".")
    return os.path.normpath(os.path.join(envdir, fname))


class TestFile(unittest.TestCase):
    def setUp(self):
        self.pathname = test_pathname("bufr/gts-acars-uk1.bufr")

    def test_named(self):
        with dballe.File(self.pathname) as f:
            self.assertEqual(f.name, self.pathname)
            self.assertEqual(f.encoding, "BUFR")

    def test_named_encoding(self):
        with dballe.File(self.pathname, "bufr") as f:
            self.assertEqual(f.name, self.pathname)
            self.assertEqual(f.encoding, "BUFR")

    def test_fileno(self):
        with open(self.pathname, "rb") as fd:
            with dballe.File(fd) as f:
                self.assertEqual(f.name, self.pathname)
                self.assertEqual(f.encoding, "BUFR")

    def test_byteio(self):
        with io.BytesIO(b"") as fd:
            with dballe.File(fd) as f:
                self.assertIn("<_io.BytesIO object at", f.name)
                self.assertEqual(f.encoding, "BUFR")


if __name__ == "__main__":
    from testlib import main
    main("test-file")
