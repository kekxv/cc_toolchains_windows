#include "cl_wrapper/arg_transformer.h"
#include <set>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

std::string ProcessClArgument(const std::string& arg)
{
  std::string arg_to_write = arg;

  if (arg_to_write.size() > 2 && (arg_to_write.substr(0, 2) == "/D" || arg_to_write.substr(0, 2) == "-D"))
  {
    std::string escaped;
    escaped.reserve(arg_to_write.size() + 4);
    for (size_t i = 0; i < arg_to_write.size(); ++i)
    {
      if (arg_to_write[i] == '"')
      {
        if (i > 0 && arg_to_write[i - 1] == '\\') escaped += '"';
        else escaped += "\\\"";
      }
      else
      {
        escaped += arg_to_write[i];
      }
    }
    arg_to_write = escaped;
  }

  bool need_quote = (arg_to_write.find(' ') != std::string::npos) && (arg_to_write.find('"') == std::string::npos);

  if (need_quote && arg_to_write != "/link")
  {
    return "\"" + arg_to_write + "\"";
  }

  return arg_to_write;
}

TransformedArgs TransformArgs(const std::vector<std::string>& args)
{
  TransformedArgs result;
  std::set<std::string> seen_args;

  for (const auto& arg : args) if (arg == "-c") result.is_compilation = true;

  size_t i = 0;
  while (i < args.size())
  {
    std::string arg = args[i];

    if (arg.find("-Wl,") == 0)
    {
    }
    else if (arg.find("-l") == 0)
    {
      std::string lib_name = arg.substr(2);
      if (!lib_name.empty()) result.args.push_back(lib_name + ".lib");
    }
    else if (arg.find("-defaultlib:") == 0 || arg.find("/defaultlib:") == 0)
    {
      size_t colon = arg.find(':');
      result.args.push_back(arg.substr(colon + 1));
    }
    else if (arg == "-o")
    {
      i++;
      if (i < args.size())
      {
        result.output_file = args[i];
        if (result.is_compilation) result.args.push_back("/Fo" + result.output_file);
        else result.args.push_back("/Fe" + result.output_file);
      }
    }
    else if (arg == "-MF")
    {
      i++;
      if (i < args.size()) result.dep_file = args[i];
    }
    else if (arg == "-MD" || arg == "-c" || arg == "-s" || arg == "-fPIC" || arg.find("-frandom-seed=") == 0)
    {
    }
    else if (arg == "-iquote" || arg == "-isystem")
    {
      i++;
      if (i < args.size()) result.args.push_back("/I" + args[i]);
    }
    else if (arg.find("-I") == 0)
    {
      if (arg.size() > 2) result.args.push_back("/I" + arg.substr(2));
      else
      {
        i++;
        if (i < args.size()) result.args.push_back("/I" + args[i]);
      }
    }
    else if (arg == "-std=c++17") result.args.push_back("/std:c++17");
    else if (arg == "-std=c++20") result.args.push_back("/std:c++20");
    else
    {
      bool is_obj_lib = false;
      std::string lower = arg;
      std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
      if (lower.size() > 4 && (lower.substr(lower.size() - 4) == ".obj" || lower.substr(lower.size() - 4) == ".lib"))
      {
        is_obj_lib = true;
      }

      if (is_obj_lib)
      {
        if (seen_args.find(arg) == seen_args.end())
        {
          result.args.push_back(arg);
          seen_args.insert(arg);
        }
      }
      else
      {
        result.args.push_back(arg);
      }
    }
    i++;
  }

  result.args.push_back("/nologo");
  result.args.push_back("/DNOMINMAX");
  result.args.push_back("/DWIN32_LEAN_AND_MEAN");
  result.args.push_back("/utf-8");

  if (result.is_compilation)
  {
    result.args.push_back("/EHsc");
    result.args.push_back("/c");
    if (!result.dep_file.empty())
    {
      result.args.push_back("/showIncludes");
    }

    bool has_rt = false;
    for (const auto& a : result.args)
    {
      if (a == "/MT" || a == "-MT" || a == "/MTd" || a == "/MD" || a == "-MD" || a == "/MDd")
      {
        has_rt = true;
        break;
      }
    }
    if (!has_rt) result.args.push_back("/MD");
  }
  else
  {
    result.args.push_back("/link");
    result.args.push_back("/SUBSYSTEM:CONSOLE");
  }

  return result;
}
