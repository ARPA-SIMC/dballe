#include <dballe++/msg.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/bufrex_codec.h>

using namespace std;

namespace dballe {

Msg::Msg()
{
	checked(dba_msg_create(&m_msg));
	m_msg->type = MSG_SYNOP;
}
Msg::~Msg()
{
	if (m_msg)
		dba_msg_delete(m_msg);
}

std::string Msg::encodeBUFR(int type, int subtype, int localsubtype)
{
	dba_msgs msgs = 0;
	dba_rawmsg rmsg = 0;
	try {
		checked(dba_msgs_create(&msgs));
		checked(dba_msgs_append_acquire(msgs, m_msg));
		checked(bufrex_encode_bufr(msgs, type, subtype, localsubtype, &rmsg));
		string res((const char*)rmsg->buf, (size_t)rmsg->len);
		msgs->msgs[0] = 0;
		dba_msgs_delete(msgs);
		dba_rawmsg_delete(rmsg);
		return res;
	} catch (...) {
		// Bit of a hack, but it's the only way not to be deallocated until we
		// add the necessary functions to the msg module
		if (msgs && msgs->len > 0)
			msgs->msgs[0] = 0;
		if (msgs) dba_msgs_delete(msgs);
		if (rmsg) dba_rawmsg_delete(rmsg);
		throw;
	}
}

}

// vim:set ts=4 sw=4:
