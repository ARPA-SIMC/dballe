import dballe
import unittest
from testlibmsg import MessageTestMixin


class TestExporter(MessageTestMixin, unittest.TestCase):
    def test_create(self):
        msg = self.make_gts_acars_uk1_message()

        exporter = dballe.Exporter("BUFR")
        binmsg = exporter.to_binary(msg)
        self.assertEqual(binmsg[:4], b"BUFR")
        self.assertEqual(binmsg[-4:], b"7777")

        binmsg1 = exporter.to_binary([msg])
        self.assertEqual(binmsg1, binmsg)

        binmsg2 = exporter.to_binary((msg, msg))
        self.assertNotEqual(binmsg2, binmsg)
        self.assertEqual(binmsg2[:4], b"BUFR")
        self.assertEqual(binmsg2[-4:], b"7777")

        binmsg3 = exporter.to_binary((msg for i in range(2)))
        self.assertEqual(binmsg3, binmsg2)

        with self.assertRaises(ValueError):
            exporter.to_binary([])


if __name__ == "__main__":
    from testlib import main
    main("test-exporter")
