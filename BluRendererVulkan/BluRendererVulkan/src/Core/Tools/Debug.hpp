#pragma once

#include <iostream>
#include <string>

#define DEBUG_ERROR(f)                                             \
  {                                                                \
    std::string res = (f);                                         \
    std::cerr << "Fatal Error : \"" << res << "\" in " << __FILE__ \
              << " at line " << __LINE__ << "\n";                  \
    exit(2401);                                                    \
  }
#define DEBUG_WARNING(f)                                                      \
  {                                                                           \
    std::string res = (f);                                                    \
    std::cout << "Warning : \"" << res << "\" in " << __FILE__ << " at line " \
              << __LINE__ << "\n";                                            \
  }