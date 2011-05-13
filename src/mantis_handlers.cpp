#include "mantis_handlers.hpp"

void abort_run(int condition)
{
  switch(condition) {
  case run_end_normal:
    std::cout << "\n\trun ended normally.  cleaning up and terminating..."
	      << std::endl;
    break;
  case run_end_killed:
    std::cout << "\n\trun termination requested.  cleaning up..."
	      << std::endl;
    break;
  default:
    break; 
  }

  end_run_and_cleanup(condition);
}
