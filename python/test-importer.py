import dballe
import unittest
from testlib import test_pathname
from testlibmsg import MessageTestMixin


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


if __name__ == "__main__":
    from testlib import main
    main("test-importer")
