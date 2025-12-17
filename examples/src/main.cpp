#include <iostream>
#include "hello.h"

int main(int argc, const char* argv[])
{
  std::string message = get_greet("Bazel & windows server");
  std::cout << message << std::endl;
  return 0;
}
