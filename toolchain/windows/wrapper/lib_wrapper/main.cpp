#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#include "common/utils.h"
#include "common/vs_env.h"
#include "common/process.h"
#include "lib_wrapper/rsp_generator.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
  SetupSystemPath();

  // Parse --arch= flag from raw args
  std::string target_arch = "x64";
  std::vector<std::string> clean_args;

  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg.find("--arch=") == 0)
    {
      target_arch = arg.substr(7);
    }
    else
    {
      clean_args.push_back(arg);
    }
  }

  // Read args from @param_file or use raw args
  std::string param_file_path;
  std::vector<std::string> args;
  if (clean_args.size() == 1 && clean_args[0][0] == '@')
  {
    param_file_path = clean_args[0].substr(1);
    std::ifstream ifs(param_file_path);
    if (ifs.is_open())
    {
      std::string line;
      while (std::getline(ifs, line))
      {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (!line.empty()) args.push_back(line);
      }
    }
  }
  else
  {
    args = clean_args;
  }

  // Find lib.exe
  std::wstring vsPath = FindVisualStudioPath();
  std::wstring libExePath = L"lib.exe";
  std::map<std::wstring, std::wstring> envMap;

  bool hasBazelVC = (GetEnvironmentVariableW(L"BAZEL_VC", NULL, 0) > 0);

  if (!hasBazelVC && !vsPath.empty())
  {
    envMap = LoadVSEnvironment(vsPath, false);
  }

  if (!vsPath.empty())
  {
    std::wstring msvcVer = FindLatestMSVCVersion(vsPath);
    if (!msvcVer.empty())
    {
      std::wstring archFolder = (target_arch == "x64") ? L"x64" : L"x86";
      fs::path p = fs::path(vsPath) / "VC" / "Tools" / "MSVC" / msvcVer / "bin" / "Hostx64" / archFolder / "lib.exe";
      if (fs::exists(p))
      {
        libExePath = p.wstring();
      }
    }
  }

  // Parse output and input files
  std::string output_file;
  std::vector<std::string> input_files;
  ParseLibArgs(args, output_file, input_files);

  // Infer output file if not found
  if (output_file.empty())
  {
    if (!param_file_path.empty())
    {
      output_file = InferOutputFromParamFile(param_file_path);
    }
    if (output_file.empty())
    {
      std::cerr << "[Error] Could not determine output file." << std::endl;
      return 1;
    }
  }

  // Ensure output directory exists
  fs::path outPath(output_file);
  if (outPath.has_parent_path())
  {
    try { fs::create_directories(outPath.parent_path()); }
    catch (...) {}
  }

  // Handle empty input: create empty .lib
  if (input_files.empty())
  {
    std::ofstream empty(output_file, std::ios::binary);
    return 0;
  }

  // Generate RSP file
  std::string unique_id = GetUUIDString();
  std::string lib_rsp_file = "lib_wrapper_" + unique_id + ".rsp";
  GenerateRspFile(lib_rsp_file, output_file, input_files);

  // Run lib.exe
  std::string cmd_line = "\"" + WStringToString(libExePath) + "\" @" + lib_rsp_file;

  ProcessResult result = RunTool(
    StringToWString(cmd_line),
    envMap,
    [](const std::string&) -> bool { return true; } // echo all lines
  );

  // Verify output exists
  if (result.exitCode == 0 && !fs::exists(output_file))
  {
    std::ofstream empty(output_file, std::ios::binary);
  }

  // Cleanup temporary RSP file
  try
  {
    if (fs::exists(lib_rsp_file)) fs::remove(lib_rsp_file);
  }
  catch (...) {}

  return result.exitCode;
}
