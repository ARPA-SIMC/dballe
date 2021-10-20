import dballe
import unittest


class TestBinaryMessage(unittest.TestCase):
    def test_create_full(self):
        msg = dballe.BinaryMessage(b"BUFR7777", "BUFR", offset=0, index=1, pathname="test.bufr")
        self.assertEqual(msg.encoding, "BUFR")
        self.assertEqual(msg.offset, 0)
        self.assertEqual(msg.index, 1)
        self.assertEqual(msg.pathname, "test.bufr")
        self.assertEqual(bytes(msg), b"BUFR7777")

    def test_create_minimal(self):
        msg = dballe.BinaryMessage(b"BUFR7777", "BUFR")
        self.assertEqual(msg.encoding, "BUFR")
        self.assertEqual(msg.offset, None)
        self.assertEqual(msg.index, None)
        self.assertEqual(msg.pathname, None)
