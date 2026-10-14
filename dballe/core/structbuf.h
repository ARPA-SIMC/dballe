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

#ifndef DBALLE_CORE_STRUCTBUF_H
#define DBALLE_CORE_STRUCTBUF_H

#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#include <wreport/error.h>

namespace dballe {

namespace structbuf {
int make_anonymous_tmpfile();
void write_buffer(int fd, void* buf, size_t size);
} // namespace structbuf

/**
 * Buffer of simple structures that becomes file backed if it grows beyond a
 * certain size.
 *
 * bufsize is the number of T items that we keep in memory before becoming
 * file-backed.
 */
template <typename T, int bufsize = 1024> class Structbuf
{
protected:
    /**
     * In-memory buffer using during appending. When it becomes full, it is
     * flushed out to a temporary file.
     */
    T* membuf = nullptr;

    /// Number of items in membuf
    unsigned membuf_last = 0;

    /**
     * Memory area used for reading. It points to membuf if we are
     * memory-backed, or it is the mmap view of the file if we are file-backed
     */
    const T* readbuf = (const T*)MAP_FAILED;

    /// Number of items appended so far
    size_t m_count = 0;

    /// Unix file descriptor to the temporary file, or -1 if we are memory
    /// backed
    int tmpfile_fd = -1;

public:
    Structbuf() : membuf(new T[bufsize]) {}
    ~Structbuf()
    {
        delete[] membuf;
        if (tmpfile_fd != -1)
        {
            if (readbuf != MAP_FAILED)
                munmap(const_cast<T*>(readbuf), m_count * sizeof(T));
            ::close(tmpfile_fd);
        }
    }

    /// Get the number of structures that have been added to the buffer so far
    size_t size() const { return m_count; }

    /// Return true if the buffer has become file-backed
    bool is_file_backed() const { return tmpfile_fd != -1; }

    /// Append an item to the buffer
    void append(const T& val)
    {
        if (readbuf != MAP_FAILED)
            throw wreport::error_consistency(
                "writing to a Structbuf that is already being read");
        if (membuf_last == bufsize)
            write_to_file();
        membuf[membuf_last++] = val;
        ++m_count;
    }

    /// Stop appending and get ready to read back the data
    void ready_to_read()
    {
        if (tmpfile_fd == -1)
            readbuf = membuf;
        else
        {
            // Flush the remaining memory data to file
            if (membuf_last)
                write_to_file();

            // mmap the file for reading
            readbuf = (const T*)mmap(nullptr, m_count * sizeof(T), PROT_READ,
                                     MAP_SHARED, tmpfile_fd, 0);
            if (readbuf == MAP_FAILED)
                throw wreport::error_system(
                    "cannot map temporary file contents to memory");
        }
    }

    /// Read back an item
    const T& operator[](size_t idx) const { return readbuf[idx]; }

protected:
    void write_to_file()
    {
        if (tmpfile_fd == -1)
            tmpfile_fd = structbuf::make_anonymous_tmpfile();
        structbuf::write_buffer(tmpfile_fd, membuf, sizeof(T) * membuf_last);
        membuf_last = 0;
    }
};

} // namespace dballe

#endif
