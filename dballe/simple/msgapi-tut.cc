#include "core/tests.h"
#include "msgapi.h"

using namespace std;
using namespace dballe;
using namespace dballe::fortran;
using namespace wibble::tests;

namespace {

typedef dballe::tests::test_group<> test_group;
typedef test_group::Test Test;
typedef test_group::Fixture Fixture;

std::vector<Test> tests {
    Test("open", [](Fixture& f) {
        // Open test file
        std::string fname = tests::datafile("bufr/simple-generic-group.bufr");
        fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

        ensure_equals(api.voglioquesto(), 4);
        ensure_equals(api.voglioquesto(), 4);
        ensure_equals(api.voglioquesto(), 4);
        ensure_equals(api.quantesono(), 1);
    }),
    Test("resume", [](Fixture& f) {
        // Test resuming after a broken BUFR

        // Concatenate a broken BUFR with a good one
        BinaryMessage rm1(read_rawmsg("bufr/interpreted-range.bufr", File::BUFR));
        BinaryMessage rm2(read_rawmsg("bufr/temp-gts1.bufr", File::BUFR));

        // Broken + good
        {
            string concat = rm1.data + rm2.data;
            FILE* out = fopen("test-simple-concat.bufr", "w");
            fwrite(concat.data(), concat.size(), 1, out);
            fclose(out);

            fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

            // The first one fails
            try {
                api.voglioquesto();
                ensure(false);
            } catch (std::exception) {
            }

            // The second one should be read
            ensure_equals(api.voglioquesto(), 555);
        }

        // Good + broken + good
        {
            string concat = rm2.data + rm1.data + rm2.data;
            FILE* out = fopen("test-simple-concat.bufr", "w");
            fwrite(concat.data(), concat.size(), 1, out);
            fclose(out);

            fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

            ensure_equals(api.voglioquesto(), 555);

            try {
                api.voglioquesto();
                ensure(false);
            } catch (std::exception) {
            }

            ensure_equals(api.voglioquesto(), 555);
        }

        // Good + broken + broken + good
        {
            string concat = rm2.data + rm1.data + rm1.data + rm2.data;
            FILE* out = fopen("test-simple-concat.bufr", "w");
            fwrite(concat.data(), concat.size(), 1, out);
            fclose(out);

            fortran::MsgAPI api("test-simple-concat.bufr", "r", "BUFR");

            ensure_equals(api.voglioquesto(), 555);

            try {
                api.voglioquesto();
                ensure(false);
            } catch (std::exception) {
            }

            try {
                api.voglioquesto();
                ensure(false);
            } catch (std::exception) {
            }

            ensure_equals(api.voglioquesto(), 555);
        }
    }),
    Test("read", [](Fixture& f) {
        // Try reading a file
        std::string fname = tests::datafile("bufr/dbapi-emptymsg.bufr");
        fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

        ensure_equals(api.voglioquesto(), 99);
        ensure_equals(api.voglioquesto(), 0);
        ensure_equals(api.voglioquesto(), 90);
        ensure_equals(api.voglioquesto(), api.missing_int);
    }),
    Test("missing", [](Fixture& f) {
        // Try reading 'missing' values
        std::string fname = tests::datafile("bufr/temp-bad5.bufr");
        fortran::MsgAPI api(fname.c_str(), "r", "BUFR");

        wassert(actual(api.enqb("latmin")) == API::missing_byte);
        //wassert(actual(api.enqc("latmin")) == API::missing_byte);
        wassert(actual(api.enqi("latmin")) == API::missing_int);
        wassert(actual(api.enqr("latmin")) == API::missing_float);
        wassert(actual(api.enqd("latmin")) == API::missing_double);

        wassert(actual(api.enqb("B05002")) == API::missing_byte);
        //wassert(actual(api.enqc("B05002")) == API::missing_byte);
        wassert(actual(api.enqi("B05002")) == API::missing_int);
        wassert(actual(api.enqr("B05002")) == API::missing_float);
        wassert(actual(api.enqd("B05002")) == API::missing_double);

        api.unsetall();

        for (unsigned msgi = 0; ; ++msgi)
        {
            WIBBLE_TEST_INFO(msgloop);
            msgloop() << "Message " << msgi;

            int count = wcallchecked(api.voglioquesto());
            if (count == API::missing_int) break;
            wassert(actual(count) > 0);

            for (unsigned i = 0; i < (unsigned)count; ++i)
            {
                msgloop() << "Message " << msgi << " var " << i;
                wrunchecked(api.dammelo());
            }
        }
#if 0
      n = 1
      do while ( n > 0 )
        call idba_voglioquesto (handle,n)
        call ensure_no_error("voglioquesto")
      
        do i = 1, n
          call idba_dammelo (handle,btable)
          call ensure_no_error("dammelo")
          call idba_enqd (handle,"B11001",dval)
          call ensure_no_error("enqd from msg")
          call idba_enqr (handle,"B11001",rval)
          call ensure_no_error("enqr from msg")
          call idba_enqi (handle,"B11001",ival)
          call ensure_no_error("enqi from msg")
          ! Value does not fit in a byte
          !call idba_enqb (handle,"B11001",bval)
          !call ensure_no_error("enqb from msg")
        end do
      end do

      call idba_fatto(handle)
      call ensure_no_error("fatto")

!     If we made it so far, exit with no error
      print*,"check_missing: all tests succeed."

      call exit (0)
    
      end program

      include "check-utils.h"
#endif
    }),
};

test_group newtg("simple_msgapi", tests);

}
