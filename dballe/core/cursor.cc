#include "cursor.h"

namespace dballe {
namespace impl {

bool CursorMessage::enqi(const char* key, unsigned len, int& res) const { return false; }
bool CursorMessage::enqd(const char* key, unsigned len, double& res) const { return false; }
bool CursorMessage::enqs(const char* key, unsigned len, std::string& res) const { return false; }
bool CursorMessage::enqf(const char* key, unsigned len, std::string& res) const { return false; }

}
}
