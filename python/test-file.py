import dballe
import io
import unittest
import os
import re


def test_pathname(fname):
    if fname.startswith("."):
        return fname

    envdir = os.environ.get("DBA_TESTDATA", ".")
    return os.path.normpath(os.path.join(envdir, fname))


class TestFile(unittest.TestCase):
    def setUp(self):
        self.pathname = test_pathname("bufr/gts-acars-uk1.bufr")

    def assertContents(self, f, pathname=None):
        if pathname is None:
            pathname = re.escape(self.pathname)
        self.assertRegex(f.name, pathname)
        self.assertEqual(f.encoding, "BUFR")
        contents = list(f)
        self.assertEqual(len(contents), 1)
        msg = contents[0]
        self.assertEqual(msg.encoding, "BUFR")
        self.assertRegex(msg.pathname, pathname)
        self.assertEqual(msg.offset, 0)
        self.assertEqual(msg.index, 0)
        data = bytes(msg)
        self.assertTrue(data.startswith(b"BUFR"))
        self.assertTrue(data.endswith(b"7777"))

    def test_named(self):
        with dballe.File(self.pathname) as f:
            self.assertContents(f)

    def test_named_encoding(self):
        with dballe.File(self.pathname, "bufr") as f:
            self.assertContents(f)

    def test_fileno(self):
        with open(self.pathname, "rb") as fd:
            with dballe.File(fd) as f:
                self.assertContents(f)

    def test_byteio(self):
        with open(self.pathname, "rb") as read_fd:
            with io.BytesIO(read_fd.read()) as fd:
                with dballe.File(fd) as f:
                    self.assertContents(f, pathname=r"^<_io\.BytesIO object at")


if __name__ == "__main__":
    from testlib import main
    main("test-file")
