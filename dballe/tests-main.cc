#include <wreport/utils/tests.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <exception>

void signal_to_exception(int)
{
    throw std::runtime_error("killing signal catched");
}

int main(int argc,const char* argv[])
{
    using namespace wreport::tests;

    signal(SIGSEGV, signal_to_exception);
    signal(SIGILL, signal_to_exception);

#if 0
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
#endif

    auto& tests = TestRegistry::get();

    SimpleTestController controller;

    if (const char* whitelist = getenv("TEST_WHITELIST"))
        controller.whitelist = whitelist;

    if (const char* blacklist = getenv("TEST_BLACKLIST"))
        controller.blacklist = blacklist;

  auto all_results = tests.run_tests(controller);

  unsigned methods_ok = 0;
  unsigned methods_failed = 0;
  unsigned methods_skipped = 0;
  unsigned test_cases_ok = 0;
  unsigned test_cases_failed = 0;

  for (const auto& tc_res: all_results)
  {
      if (!tc_res.fail_setup.empty())
      {
          fprintf(stderr, "%s: %s\n", tc_res.test_case.c_str(), tc_res.fail_setup.c_str());
          ++test_cases_failed;
      } else {
          if (!tc_res.fail_teardown.empty())
          {
              fprintf(stderr, "%s: %s\n", tc_res.test_case.c_str(), tc_res.fail_teardown.c_str());
              ++test_cases_failed;
          }
          else
              ++test_cases_ok;

          for (const auto& tm_res: tc_res.methods)
          {
              if (tm_res.skipped)
                  ++methods_skipped;
              else if (tm_res.is_success())
                  ++methods_ok;
              else
              {
                  fprintf(stderr, "\n");
                  if (tm_res.exception_typeid.empty())
                      fprintf(stderr, "%s.%s: %s\n", tm_res.test_case.c_str(), tm_res.test_method.c_str(), tm_res.error_message.c_str());
                  else
                      fprintf(stderr, "%s.%s:[%s] %s\n", tm_res.test_case.c_str(), tm_res.test_method.c_str(), tm_res.exception_typeid.c_str(), tm_res.error_message.c_str());
                  for (const auto& frame : tm_res.error_stack)
                      fprintf(stderr, "  %s", frame.format().c_str());
                  ++methods_failed;
              }
          }
      }
  }

  bool success = true;

  if (test_cases_failed)
  {
      success = false;
      fprintf(stderr, "\n%u/%u test cases had issues initializing or cleaning up\n",
              test_cases_failed, test_cases_ok + test_cases_failed);
  }

  if (methods_failed)
  {
      success = false;
      fprintf(stderr, "\n%u/%u tests failed\n", methods_failed, methods_ok + methods_failed);
  }
  else
      fprintf(stderr, "%u tests succeeded\n", methods_ok);

  return success ? 0 : 1;
}
