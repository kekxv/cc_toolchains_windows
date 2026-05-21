#pragma once

#include <string>
#include <vector>

struct TransformedArgs
{
  std::vector<std::string> args;
  bool is_compilation = false;
  std::string output_file;
  std::string dep_file;
};

// Convert GCC/Clang-style arguments to MSVC cl.exe arguments
TransformedArgs TransformArgs(const std::vector<std::string>& args);

// Quote/escape a single argument for cl.exe
std::string ProcessClArgument(const std::string& arg);
