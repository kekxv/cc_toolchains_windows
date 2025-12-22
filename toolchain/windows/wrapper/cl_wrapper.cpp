#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <unknwn.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <regex>

// =============================================================
// Visual Studio Setup Configuration COM 接口定义
// =============================================================

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "version.lib")

#ifndef __ISetupConfiguration_INTERFACE_DEFINED__
#define __ISetupConfiguration_INTERFACE_DEFINED__

const IID IID_ISetupInstance = {0xB41463C3, 0x8866, 0x43B5, {0xBC, 0x33, 0x2B, 0x06, 0x76, 0xF7, 0xF4, 0x2E}};
const IID IID_ISetupInstance2 = {0x89143C9A, 0x05AF, 0x49B0, {0xB7, 0x17, 0x72, 0xE2, 0x18, 0xA2, 0x18, 0x5C}};
const IID IID_IEnumSetupInstances = {0x6380BC6E, 0x7C73, 0x4459, {0x98, 0xC0, 0x1A, 0x31, 0x59, 0x3D, 0x29, 0x9E}};
const IID IID_ISetupConfiguration = {0x42843719, 0xDB4C, 0x46C2, {0x8E, 0x7C, 0x64, 0xF1, 0x81, 0x6D, 0xFD, 0x5B}};
const IID IID_ISetupConfiguration2 = {0x26AAB78C, 0x4A60, 0x49D6, {0xAF, 0x3B, 0x3C, 0x35, 0xBC, 0x93, 0x36, 0x5D}};
const CLSID CLSID_SetupConfiguration = {0x177F0C4A, 0x1CD3, 0x4DE7, {0xA3, 0x2C, 0x71, 0xDB, 0xBB, 0x9F, 0xA3, 0x6D}};

enum InstanceState
{
  eNone = 0, eLocal = 1, eRegistered = 2, eNoRebootRequired = 4, eNoErrors = 8, eComplete = 4294967295
};

struct ISetupInstance : public IUnknown
{
  virtual HRESULT STDMETHODCALLTYPE GetInstanceId(BSTR* pbstrInstanceId) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstallDate(LPFILETIME pInstallDate) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstallationName(BSTR* pbstrInstallationName) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstallationPath(BSTR* pbstrInstallationPath) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstallationVersion(BSTR* pbstrInstallationVersion) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetDisplayName(LCID lcid, BSTR* pbstrDisplayName) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetDescription(LCID lcid, BSTR* pbstrDescription) = 0;
  virtual HRESULT STDMETHODCALLTYPE ResolvePath(LPCOLESTR pwszRelativePath, BSTR* pbstrAbsolutePath) = 0;
};

struct ISetupInstance2 : public ISetupInstance
{
  virtual HRESULT STDMETHODCALLTYPE GetState(InstanceState* pState) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetPackages(SAFEARRAY** ppsaPackages) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetProduct(void** ppPackage) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetProductPath(BSTR* pbstrProductPath) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetErrors(void** ppErrorState) = 0;
  virtual HRESULT STDMETHODCALLTYPE IsLaunchable(VARIANT_BOOL* pfIsLaunchable) = 0;
  virtual HRESULT STDMETHODCALLTYPE IsComplete(VARIANT_BOOL* pfIsComplete) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetProperties(void** ppProperties) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetEnginePath(BSTR* pbstrEnginePath) = 0;
};

struct IEnumSetupInstances : public IUnknown
{
  virtual HRESULT STDMETHODCALLTYPE Next(ULONG celt, ISetupInstance** rgelt, ULONG* pceltFetched) = 0;
  virtual HRESULT STDMETHODCALLTYPE Skip(ULONG celt) = 0;
  virtual HRESULT STDMETHODCALLTYPE Reset(void) = 0;
  virtual HRESULT STDMETHODCALLTYPE Clone(IEnumSetupInstances** ppenum) = 0;
};

struct ISetupConfiguration : public IUnknown
{
  virtual HRESULT STDMETHODCALLTYPE EnumInstances(IEnumSetupInstances** ppEnumInstances) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstanceForCurrentProcess(ISetupInstance** ppInstance) = 0;
  virtual HRESULT STDMETHODCALLTYPE GetInstanceForPath(LPCOLESTR pwszPath, ISetupInstance** ppInstance) = 0;
};

struct ISetupConfiguration2 : public ISetupConfiguration
{
  virtual HRESULT STDMETHODCALLTYPE EnumAllInstances(IEnumSetupInstances** ppEnumInstances) = 0;
};

#endif

namespace fs = std::filesystem;

// =============================================================
// 字符串与路径工具函数
// =============================================================

std::string WStringToString(const std::wstring& wstr)
{
  if (wstr.empty()) return std::string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
  return strTo;
}

std::wstring StringToWString(const std::string& str)
{
  if (str.empty()) return std::wstring();
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
  return wstrTo;
}

// =============================================================
// [优化] 处理单个参数的转义逻辑
// =============================================================
std::string ProcessClArgument(const std::string& arg)
{
  std::string arg_to_write = arg;

  // 1. 针对宏定义 (/D 或 -D)，如果值中包含引号，必须转义为 \"
  if (arg_to_write.size() > 2 && (arg_to_write.substr(0, 2) == "/D" || arg_to_write.substr(0, 2) == "-D"))
  {
    std::string escaped;
    escaped.reserve(arg_to_write.size() + 4);
    for (size_t i = 0; i < arg_to_write.size(); ++i)
    {
      if (arg_to_write[i] == '"')
      {
        // 检查前面是否已经有转义符
        if (i > 0 && arg_to_write[i - 1] == '\\')
        {
          escaped += '"';
        }
        else
        {
          escaped += "\\\""; // 变成 \"
        }
      }
      else
      {
        escaped += arg_to_write[i];
      }
    }
    arg_to_write = escaped;
  }

  // 2. 路径/空格引号处理
  // 如果包含空格且不包含引号，且不是 "/link"，则加上双引号
  bool need_quote = (arg_to_write.find(' ') != std::string::npos) && (arg_to_write.find('"') == std::string::npos);

  if (need_quote && arg_to_write != "/link")
  {
    return "\"" + arg_to_write + "\"";
  }

  return arg_to_write;
}

// =============================================================
// 环境与查找逻辑
// =============================================================

void SetupSystemPath()
{
  wchar_t sysPath[MAX_PATH];
  wchar_t winPath[MAX_PATH];
  if (!GetSystemDirectoryW(sysPath, MAX_PATH)) wcscpy(sysPath, L"C:\\Windows\\System32");
  if (!GetWindowsDirectoryW(winPath, MAX_PATH)) wcscpy(winPath, L"C:\\Windows");

  std::wstring newPath = std::wstring(sysPath) + L";" + winPath + L";" + std::wstring(sysPath) + L"\\Wbem;";

  wchar_t* originalPathBuf = nullptr;
  size_t sz = 0;
  if (_wdupenv_s(&originalPathBuf, &sz, L"PATH") == 0 && originalPathBuf != nullptr)
  {
    newPath += originalPathBuf;
    free(originalPathBuf);
  }
  SetEnvironmentVariableW(L"PATH", newPath.c_str());
}

std::wstring FindVisualStudioPath()
{
  ISetupConfiguration* pConfig = nullptr;
  ISetupConfiguration2* pConfig2 = nullptr;
  IEnumSetupInstances* pEnum = nullptr;
  ISetupInstance* pInstance = nullptr;
  ISetupInstance2* pInstance2 = nullptr;
  HRESULT hr;

  std::wstring bestPath;
  ULONGLONG bestVersion = 0;

  hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  if (FAILED(hr)) return L"";

  hr = CoCreateInstance(CLSID_SetupConfiguration, NULL, CLSCTX_INPROC_SERVER, IID_ISetupConfiguration,
                        (LPVOID*)&pConfig);
  if (FAILED(hr))
  {
    CoUninitialize();
    std::vector<std::wstring> candidates = {
      L"C:\\Program Files\\Microsoft Visual Studio\\18\\Community",
      L"C:\\Program Files\\Microsoft Visual Studio\\18\\Enterprise",
      L"C:\\Program Files\\Microsoft Visual Studio\\18\\Professional",
      L"C:\\Program Files\\Microsoft Visual Studio\\18\\BuildTools",
      L"C:\\Program Files\\Microsoft Visual Studio\\2026\\Community",
      L"C:\\Program Files\\Microsoft Visual Studio\\2026\\Enterprise",
      L"C:\\Program Files\\Microsoft Visual Studio\\2026\\Professional",
      L"C:\\Program Files\\Microsoft Visual Studio\\2026\\BuildTools",
      L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community",
      L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise",
      L"C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional",
      L"C:\\Program Files\\Microsoft Visual Studio\\2022\\BuildTools",
      L"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community",
      L"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise",
      L"C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\BuildTools"
    };
    for (const auto& p : candidates)
    {
      if (fs::exists(p)) return p;
    }
    return L"";
  }

  hr = pConfig->QueryInterface(IID_ISetupConfiguration2, (LPVOID*)&pConfig2);
  if (SUCCEEDED(hr))
  {
    hr = pConfig2->EnumAllInstances(&pEnum);
    if (SUCCEEDED(hr))
    {
      while (pEnum->Next(1, &pInstance, NULL) == S_OK)
      {
        hr = pInstance->QueryInterface(IID_ISetupInstance2, (LPVOID*)&pInstance2);
        if (SUCCEEDED(hr))
        {
          InstanceState state;
          if (SUCCEEDED(pInstance2->GetState(&state)))
          {
            if ((state & eLocal) == eLocal)
            {
              BSTR bstrVer, bstrPath;
              if (SUCCEEDED(pInstance2->GetInstallationVersion(&bstrVer)) &&
                SUCCEEDED(pInstance2->GetInstallationPath(&bstrPath)))
              {
                std::wstring ver = bstrVer;
                std::wstring path = bstrPath;
                try
                {
                  ULONGLONG currentVerVal = std::stoull(ver.substr(0, ver.find(L'.')));
                  if (currentVerVal > bestVersion)
                  {
                    bestVersion = currentVerVal;
                    bestPath = path;
                  }
                }
                catch (...)
                {
                }
                SysFreeString(bstrVer);
                SysFreeString(bstrPath);
              }
            }
          }
          pInstance2->Release();
        }
        pInstance->Release();
      }
      pEnum->Release();
    }
    pConfig2->Release();
  }
  pConfig->Release();
  CoUninitialize();
  return bestPath;
}

std::map<std::wstring, std::wstring> LoadVSEnvironment(const std::wstring& vsPath)
{
  std::map<std::wstring, std::wstring> envMap;
#ifdef VCVARS32_
  std::wstring vcvars = vsPath + L"\\VC\\Auxiliary\\Build\\vcvars32.bat";
#else
  std::wstring vcvars = vsPath + L"\\VC\\Auxiliary\\Build\\vcvars64.bat";
#endif

  if (!fs::exists(vcvars)) return envMap;

  std::wstring cmd = L"cmd.exe /c \"\"" + vcvars + L"\" > nul && set\"";

  HANDLE hRead, hWrite;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
  if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return envMap;
  SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOW si = {sizeof(STARTUPINFOW)};
  si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  si.hStdOutput = hWrite;
  si.hStdError = NULL;
  si.wShowWindow = SW_HIDE;
  PROCESS_INFORMATION pi = {0};

  if (CreateProcessW(NULL, &cmd[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
  {
    CloseHandle(hWrite);

    std::string output;
    char buffer[4096];
    DWORD bytesRead;
    while (ReadFile(hRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
    {
      output.append(buffer, bytesRead);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hRead);

    std::stringstream ss(output);
    std::string line;
    while (std::getline(ss, line))
    {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      size_t eqPos = line.find('=');
      if (eqPos != std::string::npos)
      {
        std::string key = line.substr(0, eqPos);
        std::string val = line.substr(eqPos + 1);
        envMap[StringToWString(key)] = StringToWString(val);
      }
    }
  }
  else
  {
    CloseHandle(hRead);
    CloseHandle(hWrite);
  }
  return envMap;
}

std::vector<wchar_t> BuildEnvironmentBlock(const std::map<std::wstring, std::wstring>& envMap)
{
  std::vector<wchar_t> block;
  for (const auto& pair : envMap)
  {
    std::wstring envStr = pair.first + L"=" + pair.second;
    for (wchar_t c : envStr) block.push_back(c);
    block.push_back(0);
  }
  block.push_back(0);
  return block;
}

std::wstring FindExecutableInPath(const std::wstring& exeName, const std::wstring& pathVar)
{
  std::wstringstream ss(pathVar);
  std::wstring segment;
  while (std::getline(ss, segment, L';'))
  {
    if (segment.empty()) continue;
    try
    {
      fs::path p = fs::path(segment) / exeName;
      if (fs::exists(p))
      {
        return p.wstring();
      }
    }
    catch (...)
    {
    }
  }
  return L"";
}

std::wstring GetEnvVarCaseInsensitive(const std::map<std::wstring, std::wstring>& envMap, const std::wstring& key)
{
  std::wstring lowerKey = key;
  std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

  for (const auto& pair : envMap)
  {
    std::wstring k = pair.first;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    if (k == lowerKey) return pair.second;
  }
  return L"";
}

// =============================================================
// 参数转换逻辑
// =============================================================

struct TransformedArgs
{
  std::vector<std::string> args;
  bool is_compilation = false;
  std::string output_file;
  std::string dep_file;
};

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

int main(int argc, char* argv[])
{
  SetupSystemPath();

  std::string cwd_str = fs::current_path().string();
  fs::path tmp_dir = fs::current_path() / "_tmp";
  if (!fs::exists(tmp_dir)) fs::create_directory(tmp_dir);

  SetEnvironmentVariableW(L"TEMP", tmp_dir.c_str());
  SetEnvironmentVariableW(L"TMP", tmp_dir.c_str());
  SetEnvironmentVariableW(L"VSLANG", L"1033");

  std::wstring vsPath = FindVisualStudioPath();
  std::map<std::wstring, std::wstring> envMap;

  std::wstring clExePath = L"cl.exe";

  if (!vsPath.empty())
  {
    envMap = LoadVSEnvironment(vsPath);
    std::wstring pathVar = GetEnvVarCaseInsensitive(envMap, L"PATH");
    if (!pathVar.empty())
    {
      std::wstring foundPath = FindExecutableInPath(L"cl.exe", pathVar);
      if (!foundPath.empty())
      {
        clExePath = foundPath;
      }
    }
  }

  std::vector<std::string> raw_args;
  for (int i = 1; i < argc; ++i)
  {
    raw_args.push_back(argv[i]);
  }

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

  TransformedArgs tArgs = TransformArgs(effective_args);

  std::string clExePathUtf8 = WStringToString(clExePath);
  std::string final_cmd_line = "\"" + clExePathUtf8 + "\"";

  // =============================================================
  // [使用优化后的函数] 构建命令或生成 Response File
  // =============================================================
  if (raw_args.size() == 1 && raw_args[0][0] == '@')
  {
    std::string param_file = raw_args[0].substr(1) + ".msvc";
    std::ofstream ofs(param_file);
    for (const auto& a : tArgs.args)
    {
      // 调用抽取出的公共函数
      ofs << ProcessClArgument(a) << "\n";
    }
    ofs.close();
    final_cmd_line += " @" + param_file;
  }
  else
  {
    for (const auto& a : tArgs.args)
    {
      final_cmd_line += " ";
      // 调用抽取出的公共函数
      final_cmd_line += ProcessClArgument(a);
    }
  }

  HANDLE hStdOutRead, hStdOutWrite;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
  CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);
  SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOW si = {sizeof(STARTUPINFOW)};
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = hStdOutWrite;
  si.hStdError = hStdOutWrite;

  PROCESS_INFORMATION pi = {0};

  std::vector<wchar_t> envBlock;
  LPVOID pEnv = NULL;
  if (!envMap.empty())
  {
    wchar_t buf[32767];
    if (GetEnvVarCaseInsensitive(envMap, L"VSLANG").empty()) envMap[L"VSLANG"] = L"1033";
    if (GetEnvVarCaseInsensitive(envMap, L"SystemRoot").empty())
    {
      if (GetEnvironmentVariableW(L"SystemRoot", buf, 32767)) envMap[L"SystemRoot"] = buf;
    }
    if (GetEnvVarCaseInsensitive(envMap, L"TEMP").empty())
    {
      if (GetEnvironmentVariableW(L"TEMP", buf, 32767)) envMap[L"TEMP"] = buf;
    }
    if (GetEnvVarCaseInsensitive(envMap, L"TMP").empty())
    {
      if (GetEnvironmentVariableW(L"TMP", buf, 32767)) envMap[L"TMP"] = buf;
    }

    envBlock = BuildEnvironmentBlock(envMap);
    pEnv = envBlock.data();
  }

  std::wstring wCmdLine = StringToWString(final_cmd_line);

  BOOL success = CreateProcessW(
    NULL,
    &wCmdLine[0],
    NULL, NULL, TRUE,
    CREATE_UNICODE_ENVIRONMENT,
    pEnv,
    NULL,
    &si, &pi
  );

  CloseHandle(hStdOutWrite);

  if (!success)
  {
    std::cerr << "Failed to execute cl.exe.\nCommand: " << final_cmd_line << "\nError: " << GetLastError() << std::endl;
    return 1;
  }

  std::vector<std::string> includes;
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

  std::string buffer_acc;
  char buffer[4096];
  DWORD bytesRead;

  while (ReadFile(hStdOutRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
  {
    buffer_acc.append(buffer, bytesRead);
    size_t pos;
    while ((pos = buffer_acc.find('\n')) != std::string::npos)
    {
      std::string line = buffer_acc.substr(0, pos);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      buffer_acc.erase(0, pos + 1);

      if (tArgs.is_compilation && (line.find("including file:") != std::string::npos || line.find("包含文件:") !=
        std::string::npos))
      {
        size_t fileMarker = line.find("file:");
        if (fileMarker != std::string::npos)
        {
          std::string pathStr = line.substr(fileMarker + 5);
          size_t firstNonSpace = pathStr.find_first_not_of(" ");
          if (firstNonSpace != std::string::npos) pathStr = pathStr.substr(firstNonSpace);

          try
          {
            fs::path absP = fs::absolute(pathStr);
            fs::path relP = fs::relative(absP, fs::current_path());
            if (relP.string().find("..") == 0 || relP.string().find(':') != std::string::npos)
            {
              includes.push_back(absP.string());
            }
            else
            {
              includes.push_back(relP.string());
            }
          }
          catch (...)
          {
            includes.push_back(pathStr);
          }
        }
      }
      else if (line.find("D9025") != std::string::npos || line.find("D9014") != std::string::npos || line.find("D9002")
        != std::string::npos)
      {
        // ignore
      }
      else if (!source_file_name.empty() && line == source_file_name)
      {
        // ignore echo of source filename
      }
      else
      {
        std::cout << line << std::endl;
      }
    }
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode = 0;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(hStdOutRead);

  if (exitCode == 0 && !tArgs.is_compilation && !tArgs.output_file.empty())
  {
    std::string lower_out = tArgs.output_file;
    std::transform(lower_out.begin(), lower_out.end(), lower_out.begin(), ::tolower);
    if (lower_out.find(".exe") == std::string::npos)
    {
      std::string exe_name = tArgs.output_file + ".exe";
      if (fs::exists(exe_name) && !fs::exists(tArgs.output_file))
      {
        try
        {
          fs::copy_file(exe_name, tArgs.output_file, fs::copy_options::overwrite_existing);
        }
        catch (...)
        {
        }
      }
    }
  }

  if (exitCode == 0 && tArgs.is_compilation && !tArgs.dep_file.empty() && !tArgs.output_file.empty())
  {
    std::ofstream dofs(tArgs.dep_file);
    if (dofs.is_open())
    {
      std::string out = tArgs.output_file;
      std::replace(out.begin(), out.end(), '\\', '/');
      std::string esc_out;
      for (char c : out)
      {
        if (c == ' ') esc_out += "\\ ";
        else esc_out += c;
      }

      dofs << esc_out << ":";

      for (auto inc : includes)
      {
        std::replace(inc.begin(), inc.end(), '\\', '/');
        std::string esc_inc;
        for (char c : inc)
        {
          if (c == ' ') esc_inc += "\\ ";
          else esc_inc += c;
        }
        dofs << " \\\n  " << esc_inc;
      }
      dofs << "\n";
    }
  }

  return exitCode;
}
