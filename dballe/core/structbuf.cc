/*
 * core/structbuf - memory or file-backed storage of structures
 *
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */
#include "structbuf.h"
#include <cerrno>
#include <wreport/error.h>

using namespace std;
using namespace wreport;

namespace dballe {
namespace structbuf {

int make_anonymous_tmpfile()
{
    const char* tmpdir = getenv("TMPDIR");
    if (!tmpdir)
        tmpdir = "/tmp";
    std::string tmpnam = tmpdir;
    tmpnam += "/dballe-XXXXXX";

    // FIXME: to avoid a string copy, we are writing directly to the string
    // buffer
    int fd = mkstemp(const_cast<char*>(tmpnam.c_str()));
    if (fd == -1)
        error_system::throwf("cannot create temporary file in %s", tmpdir);

    if (unlink(tmpnam.c_str()) == -1)
    {
        int orig_errno = errno;
        close(fd);
        throw error_system("cannot remove newly created temporary file",
                           orig_errno);
    }

    return fd;
}

void write_buffer(int fd, void* buf, size_t size)
{
    ssize_t res = ::write(fd, buf, size);
    if (res == -1)
        throw error_system("cannot write to temporary file");
    if ((size_t)res < size)
        throw error_consistency("write to temporary file was interrupted");
}

} // namespace structbuf
} // namespace dballe
