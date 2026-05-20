#include "cl_wrapper/show_args.h"
#include "common/utils.h"

#include <iostream>
#include <windows.h>

bool HandleBazelShowArgs(int argc, char* argv[],
                         const std::map<std::wstring, std::wstring>& envMap)
{
  for (int i = 1; i < argc; ++i)
  {
    std::string currentArg = argv[i];
    std::string prefix = "/bazel-show-args=";

    if (currentArg.size() <= prefix.size() || currentArg.substr(0, prefix.size()) != prefix)
      continue;

    std::string configName = currentArg.substr(prefix.size());
    std::vector<std::wstring> keys = {
      L"BAZEL_VC", L"PATH", L"INCLUDE", L"LIB", L"LIBPATH",
      L"WindowsSdkDir", L"WindowsLibPath", L"UniversalCRTSdkDir",
      L"UCRTVersion", L"VCINSTALLDIR", L"VisualStudioVersion"
    };

    auto getValue = [&](const std::wstring& k) -> std::wstring
    {
      if (!envMap.empty())
      {
        std::wstring val = GetEnvVarCaseInsensitive(envMap, k);
        if (!val.empty()) return val;
      }
      wchar_t buf[32767];
      if (GetEnvironmentVariableW(k.c_str(), buf, 32767)) return std::wstring(buf);
      return L"";
    };

    auto escapeBackslashes = [](const std::string& s) -> std::string
    {
      std::string res;
      res.reserve(s.size() + 16);
      for (char c : s)
      {
        if (c == '\\') res += "\\\\";
        else res += c;
      }
      return res;
    };

    for (const auto& key : keys)
    {
      std::wstring val = getValue(key);
      if (!val.empty())
      {
        std::string valUtf8 = WStringToString(val);
        std::cout << "build:" << configName << " --action_env=\""
          << WStringToString(key) << "=" << escapeBackslashes(valUtf8)
          << "\"" << std::endl;
      }
    }
    return true;
  }
  return false;
}
