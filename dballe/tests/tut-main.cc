/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <tut.h>
#include <tut_reporter.h>
#include <signal.h>

namespace tut
{
  test_runner_singleton runner;
}

void signal_to_exception(int)          
{
	  throw std::runtime_error("killing signal catched");
}

int main(int argc,const char* argv[])
{
  tut::reporter visi;

  signal(SIGSEGV,signal_to_exception);         
  signal(SIGILL,signal_to_exception);

  if( (argc == 2 && (! strcmp ("help", argv[1]))) || argc > 3 )
  {
    std::cout << "TUT example test application." << std::endl;
    std::cout << "Usage: example [regression] | [list] | [ group] [test]" << std::endl;
    std::cout << "       List all groups: example list" << std::endl;
    std::cout << "       Run all tests: example regression" << std::endl;
    std::cout << "       Run one group: example std::auto_ptr" << std::endl;
    std::cout << "       Run one test: example std::auto_ptr 3" << std::endl;;
  }

  // std::cout << "\nFAILURE and EXCEPTION in these tests are FAKE ;)\n\n";

  tut::runner.get().set_callback(&visi);

  try
  {
    if( argc == 1 || (argc == 2 && std::string(argv[1]) == "regression") )
    {
      tut::runner.get().run_tests();
    }
    else if( argc == 2 && std::string(argv[1]) == "list" )
    {
      std::cout << "registered test groups:" << std::endl;
      tut::groupnames gl = tut::runner.get().list_groups();
      tut::groupnames::const_iterator i = gl.begin();
      tut::groupnames::const_iterator e = gl.end();
      while( i != e )
      {
        std::cout << "  " << *i << std::endl;
        ++i;
      }
    }
    else if( argc == 2 && std::string(argv[1]) != "regression" )
    {
      tut::runner.get().run_tests(argv[1]);
    }
    else if( argc == 3 )
    {
      tut::runner.get().run_test(argv[1],::atoi(argv[2]));
    }
  }
  catch( const std::exception& ex )
  {
    std::cerr << "tut raised exception: " << ex.what() << std::endl;
  }

  return 0;
}
