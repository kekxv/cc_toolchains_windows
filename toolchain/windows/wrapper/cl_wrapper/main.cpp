#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#include "common/utils.h"
#include "common/vs_env.h"
#include "common/process.h"
#include "cl_wrapper/arg_transformer.h"
#include "cl_wrapper/depfile.h"
#include "cl_wrapper/show_args.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
  SetupSystemPath();

  std::string cwd_str = fs::current_path().string();
  fs::path tmp_dir = fs::current_path() / "_tmp";
  if (!fs::exists(tmp_dir)) fs::create_directory(tmp_dir);

  SetEnvironmentVariableW(L"TEMP", tmp_dir.c_str());
  SetEnvironmentVariableW(L"TMP", tmp_dir.c_str());
  SetEnvironmentVariableW(L"VSLANG", L"1033");

  // Check if VS environment is already available (INCLUDE env + cl.exe in PATH)
  wchar_t checkInclude[32768];
  bool hasIncludeEnv = (GetEnvironmentVariableW(L"INCLUDE", checkInclude, 32768) > 0);

  wchar_t sysPathBuf[32768];
  GetEnvironmentVariableW(L"PATH", sysPathBuf, 32768);
  std::wstring clExePath = FindExecutableInPath(L"cl.exe", sysPathBuf);

  bool envMatchesTarget = true;
  if (hasIncludeEnv && !clExePath.empty())
  {
    try
    {
      fs::path p(clExePath);
      std::wstring targetDirName = p.parent_path().filename().wstring();
      std::transform(targetDirName.begin(), targetDirName.end(), targetDirName.begin(), ::tolower);
      if (targetDirName == L"x64" || targetDirName == L"x86")
      {
#ifdef VCVARS32_
        if (targetDirName != L"x86") envMatchesTarget = false;
#else
        if (targetDirName != L"x64") envMatchesTarget = false;
#endif
      }
      else
      {
        // Fallback: check VSCMD_ARG_TGT_ARCH environment variable
        wchar_t tgtArchBuf[256];
        if (GetEnvironmentVariableW(L"VSCMD_ARG_TGT_ARCH", tgtArchBuf, 256) > 0)
        {
          std::wstring tgtArch = tgtArchBuf;
          std::transform(tgtArch.begin(), tgtArch.end(), tgtArch.begin(), ::tolower);
#ifdef VCVARS32_
          if (tgtArch != L"x86") envMatchesTarget = false;
#else
          if (tgtArch != L"x64") envMatchesTarget = false;
#endif
        }
      }
    }
    catch (...)
    {
    }
  }

  std::map<std::wstring, std::wstring> envMap;

  if (!hasIncludeEnv || clExePath.empty() || !envMatchesTarget)
  {
    wchar_t bazelVCBuf[MAX_PATH];
    if (GetEnvironmentVariableW(L"BAZEL_VC", bazelVCBuf, MAX_PATH) > 0)
    {
#ifdef VCVARS32_
      envMap = LoadVSEnvironment(std::wstring(bazelVCBuf), true);
#else
      envMap = LoadVSEnvironment(std::wstring(bazelVCBuf), false);
#endif
    }

    if (envMap.empty())
    {
      std::wstring vsPath = FindVisualStudioPath();
      if (!vsPath.empty())
      {
#ifdef VCVARS32_
        envMap = LoadVSEnvironment(vsPath, true);
#else
        envMap = LoadVSEnvironment(vsPath, false);
#endif
      }
    }

    if (!envMap.empty())
    {
      std::wstring newPathVar = GetEnvVarCaseInsensitive(envMap, L"PATH");
      std::wstring found = FindExecutableInPath(L"cl.exe", newPathVar);
      if (!found.empty()) clExePath = found;
    }
  }

  if (clExePath.empty())
  {
    clExePath = L"cl.exe";
  }

  // Handle /bazel-show-args= early exit
  if (HandleBazelShowArgs(argc, argv, envMap))
    return 0;

  // Read arguments
  std::vector<std::string> raw_args;
  for (int i = 1; i < argc; ++i) raw_args.push_back(argv[i]);

  std::vector<std::string> effective_args;
  if (raw_args.size() == 1 && raw_args[0][0] == '@')
  {
    std::string param_file = raw_args[0].substr(1);
    std::ifstream ifs(param_file);
    if (ifs.is_open())
    {
      std::string line;
      while (std::getline(ifs, line))
      {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        if (!line.empty()) effective_args.push_back(line);
      }
    }
  }
  else
  {
    effective_args = raw_args;
  }

  // Find source file name for output filtering
  std::string source_file_name;
  for (const auto& arg : effective_args)
  {
    if (arg[0] != '-' && arg[0] != '/')
    {
      std::string ext = fs::path(arg).extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      if (ext == ".c" || ext == ".cpp" || ext == ".cc" || ext == ".cxx")
      {
        source_file_name = fs::path(arg).filename().string();
        break;
      }
    }
  }

  // Transform arguments
  TransformedArgs tArgs = TransformArgs(effective_args);

  // Build command line
  std::string clExePathUtf8 = WStringToString(clExePath);
  std::string final_cmd_line = "\"" + clExePathUtf8 + "\"";

  if (raw_args.size() == 1 && raw_args[0][0] == '@')
  {
    std::string param_file = raw_args[0].substr(1) + ".msvc";
    std::ofstream ofs(param_file);
    for (const auto& a : tArgs.args) ofs << ProcessClArgument(a) << "\n";
    ofs.close();
    final_cmd_line += " @" + param_file;
  }
  else
  {
    for (const auto& a : tArgs.args)
    {
      final_cmd_line += " ";
      final_cmd_line += ProcessClArgument(a);
    }
  }

  // Enrich envMap with fallback values
  if (!envMap.empty())
  {
    wchar_t buf[32767];
    if (GetEnvVarCaseInsensitive(envMap, L"VSLANG").empty()) envMap[L"VSLANG"] = L"1033";
    if (GetEnvVarCaseInsensitive(envMap, L"TEMP").empty())
    {
      if (GetEnvironmentVariableW(L"TEMP", buf, 32767)) envMap[L"TEMP"] = buf;
    }
    if (GetEnvVarCaseInsensitive(envMap, L"TMP").empty())
    {
      if (GetEnvironmentVariableW(L"TMP", buf, 32767)) envMap[L"TMP"] = buf;
    }
  }

  // Collect includes for depfile generation
  std::vector<std::string> includes;

  // Run cl.exe with line filtering
  ProcessResult result = RunTool(
    StringToWString(final_cmd_line),
    envMap,
    [&](const std::string& line) -> bool
    {
      // Parse /showIncludes output
      if (tArgs.is_compilation)
      {
        if (ParseShowIncludes(line, includes))
          return false; // consumed, don't print
      }

      // Filter MSVC warnings
      if (line.find("D9025") != std::string::npos ||
          line.find("D9014") != std::string::npos ||
          line.find("D9002") != std::string::npos)
        return false;

      // Filter bare source filename line
      if (!source_file_name.empty() && line == source_file_name)
        return false;

      return true;
    }
  );

  // Post-compilation: fix .exe extension for link output
  if (result.exitCode == 0 && !tArgs.is_compilation && !tArgs.output_file.empty())
  {
    std::string lower_out = tArgs.output_file;
    std::transform(lower_out.begin(), lower_out.end(), lower_out.begin(), ::tolower);
    if (lower_out.find(".exe") == std::string::npos)
    {
      std::string exe_name = tArgs.output_file + ".exe";
      if (fs::exists(exe_name) && !fs::exists(tArgs.output_file))
      {
        try { fs::copy_file(exe_name, tArgs.output_file, fs::copy_options::overwrite_existing); }
        catch (...) {}
      }
    }
  }

  // Post-compilation: write depfile
  if (result.exitCode == 0 && tArgs.is_compilation && !tArgs.dep_file.empty() && !tArgs.output_file.empty())
  {
    WriteDepFile(tArgs.dep_file, tArgs.output_file, includes);
  }

  return result.exitCode;
}
