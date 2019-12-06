import dballe
import unittest
from testlib import test_pathname
from testlibmsg import MessageTestMixin
import io
import sys


class TestImporter(MessageTestMixin, unittest.TestCase):
    def test_default(self):
        pathname = test_pathname("bufr/gts-acars-uk1.bufr")
        with dballe.File(pathname) as f:
            binmsg = next(f)

        importer = dballe.Importer("BUFR")
        msg = importer.from_binary(binmsg)
        self.assertEqual(len(msg), 1)
        self.assert_gts_acars_uk1_contents(msg[0])

    def test_simplified(self):
        pathname = test_pathname("bufr/gts-acars-uk1.bufr")
        with dballe.File(pathname) as f:
            binmsg = next(f)

        importer = dballe.Importer("BUFR", simplified=True)
        msg = importer.from_binary(binmsg)
        self.assertEqual(len(msg), 1)
        self.assert_gts_acars_uk1_contents(msg[0])

    def test_not_simplified(self):
        pathname = test_pathname("bufr/gts-acars-uk1.bufr")
        with dballe.File(pathname) as f:
            binmsg = next(f)

        importer = dballe.Importer("BUFR", simplified=False)
        msg = importer.from_binary(binmsg)
        self.assertEqual(len(msg), 1)
        self.assert_gts_acars_uk1_contents(msg[0])

    def test_fromfile(self):
        pathname = test_pathname("bufr/gts-acars-uk1.bufr")
        importer = dballe.Importer("BUFR")

        with dballe.File(pathname) as f:
            decoded = list(importer.from_file(f))

        self.assertEqual(len(decoded), 1)
        self.assertEqual(len(decoded[0]), 1)
        msg = decoded[0][0]
        self.assert_gts_acars_uk1_contents(msg)

    def test_fromfile_shortcut_pathname(self):
        pathname = test_pathname("bufr/gts-acars-uk1.bufr")
        importer = dballe.Importer("BUFR")

        with importer.from_file(pathname) as f:
            decoded = list(f)

        self.assertEqual(len(decoded), 1)
        self.assertEqual(len(decoded[0]), 1)
        msg = decoded[0][0]
        self.assert_gts_acars_uk1_contents(msg)

    def test_fromfile_shortcut_file(self):
        pathname = test_pathname("bufr/gts-acars-uk1.bufr")
        importer = dballe.Importer("BUFR")

        with open(pathname, "rb") as fd:
            with importer.from_file(fd) as f:
                decoded = list(f)

        self.assertEqual(len(decoded), 1)
        self.assertEqual(len(decoded[0]), 1)
        msg = decoded[0][0]
        self.assert_gts_acars_uk1_contents(msg)

    def test_fromfile_shortcut_byteio(self):
        pathname = test_pathname("bufr/gts-acars-uk1.bufr")
        importer = dballe.Importer("BUFR")

        with open(pathname, "rb") as fd:
            data = fd.read()

        with io.BytesIO(data) as fd:
            with importer.from_file(fd) as f:
                decoded = list(f)

        self.assertEqual(len(decoded), 1)
        self.assertEqual(len(decoded[0]), 1)
        msg = decoded[0][0]
        self.assert_gts_acars_uk1_contents(msg)

    def test_refcounts(self):
        pathname = test_pathname("bufr/gts-acars-uk1.bufr")
        importer = dballe.Importer("BUFR")
        self.assertEqual(sys.getrefcount(importer), 2)  # importer, getrefcount

        with dballe.File(pathname) as f:
            self.assertEqual(sys.getrefcount(importer), 2)  # importer, getrefcount
            self.assertEqual(sys.getrefcount(f), 3)  # __enter__ result, f, getrefcount
            fimp = importer.from_file(f)
            self.assertEqual(sys.getrefcount(importer), 3)  # importer, fimp, getrefcount
            self.assertEqual(sys.getrefcount(f), 4)  # __enter__ result, f, fimp, getrefcount
            decoded = list(fimp)
            self.assertEqual(sys.getrefcount(importer), 3)  # importer, fimp, getrefcount
            self.assertEqual(sys.getrefcount(f), 4)  # __enter__ result, f, fimp, getrefcount

        self.assertEqual(sys.getrefcount(importer), 3)  # importer, fimp, getrefcount
        self.assertEqual(sys.getrefcount(f), 3)  # f, fimp, getrefcount
        self.assertEqual(len(decoded), 1)

    def test_issue197(self):
        pathname = test_pathname("bufr/gts-acars-uk1.bufr")

        with dballe.File(pathname) as f:
            binmsg = next(f)

        importer = dballe.Importer("BUFR")
        msgs = importer.from_binary(binmsg)
        with msgs[0].query_data() as m:
            with self.assertRaises(RuntimeError) as e:
                m.data
            self.assertEqual(str(e.exception), "cannot access values on a cursor before or after iteration")


if __name__ == "__main__":
    from testlib import main
    main("test-importer")
