#include "hello.h"
#include <iostream>

std::string get_greet(const std::string& name)
{
  return "Hello, " + name + "! (from Windows Static Lib)";
}
