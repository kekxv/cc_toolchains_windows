#include "lib_wrapper/rsp_generator.h"

#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <objbase.h>
#include <ole2.h>

#include "common/utils.h"

namespace fs = std::filesystem;

std::string GetUUIDString()
{
  GUID guid;
  HRESULT hr = CoCreateGuid(&guid);
  if (FAILED(hr)) return "temp_lib_rsp";

  wchar_t guidStr[40];
  StringFromGUID2(guid, guidStr, 40);
  std::wstring ws(guidStr);
  std::string s = WStringToString(ws);
  std::string res;
  for (char c : s)
  {
    if (isalnum(c)) res += c;
  }
  return res;
}

std::string ParseLibArgs(const std::vector<std::string>& args,
                         std::string& output_file,
                         std::vector<std::string>& input_files)
{
  std::string target_arch = "x64";

  for (const auto& arg : args)
  {
    if (arg.find("--arch=") == 0)
    {
      target_arch = arg.substr(7);
      continue;
    }

    if (arg.empty()) continue;

    bool is_flag = false;

    // MSVC-style flags (/nologo, /out:..., /machine:...)
    if (arg[0] == '/')
    {
      if (arg.size() > 5)
      {
        std::string prefix = arg.substr(0, 5);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
        if (prefix == "/out:")
        {
          output_file = arg.substr(5);
          std::replace(output_file.begin(), output_file.end(), '/', '\\');
          continue;
        }
      }
      is_flag = true;
    }

    // ar-style flags (rcsq)
    if (arg.find_first_of("/\\") == std::string::npos &&
        arg.find(".lib") == std::string::npos &&
        arg.find(".obj") == std::string::npos)
    {
      bool has_rcsq = false;
      for (char c : arg)
      {
        if (std::string("rcsq").find(c) != std::string::npos)
        {
          has_rcsq = true;
          break;
        }
      }
      if (has_rcsq) is_flag = true;
    }

    if (arg[0] == '-') is_flag = true;
    if (is_flag) continue;

    std::string fixed_arg = arg;
    std::replace(fixed_arg.begin(), fixed_arg.end(), '/', '\\');

    if (output_file.empty())
    {
      std::string lower_arg = fixed_arg;
      std::transform(lower_arg.begin(), lower_arg.end(), lower_arg.begin(), ::tolower);
      bool is_obj = false;
      if (lower_arg.size() > 4 && lower_arg.substr(lower_arg.size() - 4) == ".obj") is_obj = true;
      else if (lower_arg.size() > 2 && lower_arg.substr(lower_arg.size() - 2) == ".o") is_obj = true;
      else if (lower_arg.size() > 3 && lower_arg.substr(lower_arg.size() - 3) == ".lo") is_obj = true;

      if (is_obj)
      {
        input_files.push_back(fixed_arg);
      }
      else
      {
        output_file = fixed_arg;
      }
    }
    else
    {
      input_files.push_back(fixed_arg);
    }
  }

  return target_arch;
}

void GenerateRspFile(const std::string& rsp_path,
                     const std::string& output_file,
                     const std::vector<std::string>& input_files)
{
  std::ofstream rsp(rsp_path);
  if (!rsp.is_open())
  {
    std::cerr << "[Error] Failed to write response file." << std::endl;
    return;
  }
  rsp << "/OUT:" << output_file << "\n";
  rsp << "/NOLOGO\n";
  rsp << "/IGNORE:4006\n";
  rsp << "/IGNORE:4221\n";

  for (const auto& f : input_files)
  {
    if (f.find(' ') != std::string::npos)
    {
      rsp << "\"" << f << "\"\n";
    }
    else
    {
      rsp << f << "\n";
    }
  }
}

std::string InferOutputFromParamFile(const std::string& param_file_path)
{
  if (param_file_path.find(".params") == std::string::npos)
    return "";

  std::string candidate = param_file_path.substr(0, param_file_path.find(".params"));
  if (candidate.size() > 2 && candidate[candidate.size() - 2] == '-' && isdigit(candidate.back()))
  {
    candidate = candidate.substr(0, candidate.size() - 2);
  }
  std::cout << "[Warning] Inferring output file: " << candidate << std::endl;
  std::replace(candidate.begin(), candidate.end(), '/', '\\');
  return candidate;
}
