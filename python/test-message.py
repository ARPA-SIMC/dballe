import dballe
import unittest
from testlibmsg import MessageTestMixin


class TestMessage(MessageTestMixin, unittest.TestCase):
    def test_empty(self):
        msg = dballe.Message("synop")
        self.assertEqual(str(msg), "Message")
        self.assertEqual(repr(msg), "dballe.Message object")
        self.assertEqual(msg.type, "synop")
        self.assertIsNone(msg.datetime)
        self.assertIsNone(msg.coords)
        self.assertIsNone(msg.ident)
        self.assertEqual(msg.report, "synop")

    def test_create(self):
        msg = self.make_gts_acars_uk1_message()
        self.assert_gts_acars_uk1_contents(msg)

        # auto msgs = read_msgs("bufr/gts-acars-uk1.bufr", Encoding::BUFR);
        # wassert(actual(msg->diff(*msgs[0])) == 0);


if __name__ == "__main__":
    from testlib import main
    main("test-message")
