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
// COM 接口定义 (用于查找 Visual Studio)
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
// 工具函数
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

// 获取不带连字符的 UUID 用于临时文件名
std::string GetUUIDString()
{
  GUID guid;
  HRESULT hr = CoCreateGuid(&guid);
  if (FAILED(hr)) return "temp_lib_rsp";

  wchar_t guidStr[40];
  StringFromGUID2(guid, guidStr, 40);
  std::wstring ws(guidStr);
  // Remove { } and -
  std::string s = WStringToString(ws);
  std::string res;
  for (char c : s)
  {
    if (isalnum(c)) res += c;
  }
  return res;
}

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

// 查找 MSVC 目录下的最新版本文件夹
std::wstring FindLatestMSVCVersion(const std::wstring& vsPath)
{
  fs::path msvcRoot = fs::path(vsPath) / "VC" / "Tools" / "MSVC";
  std::wstring bestVerStr;
  if (fs::exists(msvcRoot))
  {
    for (const auto& entry : fs::directory_iterator(msvcRoot))
    {
      if (entry.is_directory())
      {
        std::wstring verStr = entry.path().filename().wstring();
        // 简单的字符串比较通常对版本号有效 (如果位数对齐)，或者可以做更复杂的解析
        // "14.44.35207" > "14.30..."
        if (verStr > bestVerStr)
        {
          bestVerStr = verStr;
        }
      }
    }
  }
  return bestVerStr;
}

std::map<std::wstring, std::wstring> LoadVSEnvironment(const std::wstring& vsPath)
{
  std::map<std::wstring, std::wstring> envMap;
  // lib.exe 通常不需要完整的 vcvars64 复杂环境，但为了保险依然加载
  std::wstring vcvars = vsPath + L"\\VC\\Auxiliary\\Build\\vcvars64.bat";
  if (!fs::exists(vcvars)) return envMap;

  std::wstring cmd = L"cmd.exe /c \"\"" + vcvars + L"\" > nul && set\"";

  HANDLE hRead, hWrite;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
  if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return envMap;
  SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOW si = {sizeof(STARTUPINFOW)};
  si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
  si.hStdOutput = hWrite;
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

// =============================================================
// 主逻辑
// =============================================================

int main(int argc, char* argv[])
{
  SetupSystemPath();

  std::string target_arch = "x64";
  std::vector<std::string> clean_args;

  // 1. 处理架构参数和参数文件
  std::vector<std::string> raw_argv;
  // 从 argv[1] 开始
  for (int i = 1; i < argc; ++i)
  {
    raw_argv.push_back(argv[i]);
  }

  for (const auto& arg : raw_argv)
  {
    if (arg.find("--arch=") == 0)
    {
      target_arch = arg.substr(7);
    }
    else
    {
      clean_args.push_back(arg);
    }
  }

  // 处理 @param_file
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

  // 2. 寻找 lib.exe
  std::wstring vsPath = FindVisualStudioPath();
  std::wstring libExePath = L"lib.exe"; // 默认 fallback
  std::map<std::wstring, std::wstring> envMap;

  if (!vsPath.empty())
  {
    envMap = LoadVSEnvironment(vsPath); // 加载基础环境，主要是为了 DLL
    std::wstring msvcVer = FindLatestMSVCVersion(vsPath);
    if (!msvcVer.empty())
    {
      // 构造路径: VC/Tools/MSVC/<ver>/bin/Hostx64/<arch>/lib.exe
      // 注意: 构建机通常是 Hostx64
      std::wstring archFolder = (target_arch == "x64") ? L"x64" : L"x86";
      fs::path p = fs::path(vsPath) / "VC" / "Tools" / "MSVC" / msvcVer / "bin" / "Hostx64" / archFolder / "lib.exe";
      if (fs::exists(p))
      {
        libExePath = p.wstring();
      }
    }
  }

  // 3. 解析 Output 和 Input
  std::string output_file;
  std::vector<std::string> input_files;

  for (const auto& arg : args)
  {
    if (arg.empty()) continue;

    bool is_flag = false;

    // 【新增修复】：处理 MSVC 风格的参数 (/nologo, /out:..., /machine:...)
    if (arg[0] == '/')
    {
      // 检查是否是 /out: 或 /OUT:
      if (arg.size() > 5)
      {
        std::string prefix = arg.substr(0, 5);
        // 简单的忽略大小写比较
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::tolower);
        if (prefix == "/out:")
        {
          output_file = arg.substr(5);
          std::replace(output_file.begin(), output_file.end(), '/', '\\');
          continue; // 这是输出文件参数，处理完后跳过
        }
      }

      // 如果是其他以 / 开头的参数（如 /nologo, /machine:x64），视为 flag 跳过
      // 注意：如果你的输入文件路径可能以 / 开头（例如 /usr/lib/...），这可能会误判。
      // 但在 Windows 环境下配合 lib.exe，以 / 开头的通常是 flag。
      is_flag = true;
    }

    // 兼容原有的逻辑：处理 ar 风格的参数
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

    // 如果 output_file 还没定，且当前不是 /out: 指定的（因为上面已经continue了），
    // 这里的逻辑通常是针对 positional arguments（位置参数）的。
    // 如果你的调用方严格使用了 /out:，这里的 output_file 赋值逻辑主要是作为 fallback。
    if (output_file.empty())
    {
      output_file = fixed_arg;
    }
    else
    {
      input_files.push_back(fixed_arg);
    }
  }

  // 推断输出文件 (如果参数中没找到)
  if (output_file.empty())
  {
    if (!param_file_path.empty() && param_file_path.find(".params") != std::string::npos)
    {
      std::string candidate = param_file_path.substr(0, param_file_path.find(".params"));
      // 移除 bazel 可能生成的 -2.params 这种后缀
      if (candidate.size() > 2 && candidate[candidate.size() - 2] == '-' && isdigit(candidate.back()))
      {
        candidate = candidate.substr(0, candidate.size() - 2);
      }
      std::cout << "[Warning] Inferring output file: " << candidate << std::endl;
      output_file = candidate;
      std::replace(output_file.begin(), output_file.end(), '/', '\\');
    }
    else
    {
      std::cerr << "[Error] Could not determine output file." << std::endl;
      return 1;
    }
  }

  // 确保输出目录存在
  fs::path outPath(output_file);
  if (outPath.has_parent_path())
  {
    try { fs::create_directories(outPath.parent_path()); }
    catch (...)
    {
    }
  }

  // 处理空输入: 创建空文件并退出
  if (input_files.empty())
  {
    std::ofstream empty(output_file, std::ios::binary);
    return 0;
  }

  // 4. 生成 RSP 文件
  std::string unique_id = GetUUIDString();
  std::string lib_rsp_file = "lib_wrapper_" + unique_id + ".rsp";

  {
    // 必须以 MBCS (ANSI) 或 UTF-8 写入，VS 工具通常能处理
    std::ofstream rsp(lib_rsp_file);
    if (!rsp.is_open())
    {
      std::cerr << "[Error] Failed to write response file." << std::endl;
      return 1;
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

  // 5. 执行 lib.exe
  std::string cmd_line = "\"" + WStringToString(libExePath) + "\" @" + lib_rsp_file;
  std::wstring wCmdLine = StringToWString(cmd_line);

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
    // 补充必要的 SystemRoot 等
    if (envMap.find(L"SystemRoot") == envMap.end())
    {
      if (GetEnvironmentVariableW(L"SystemRoot", buf, 32767)) envMap[L"SystemRoot"] = buf;
    }
    envBlock = BuildEnvironmentBlock(envMap);
    pEnv = envBlock.data();
  }

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
    std::cerr << "Failed to execute lib.exe.\nCommand: " << cmd_line << "\nError: " << GetLastError() << std::endl;
    return 1;
  }

  // 读取输出
  char buffer[4096];
  DWORD bytesRead;
  while (ReadFile(hStdOutRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
  {
    std::cout.write(buffer, bytesRead);
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exitCode = 0;
  GetExitCodeProcess(pi.hProcess, &exitCode);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(hStdOutRead);

  // 6. 清理和收尾
  // 如果成功但没有生成文件（极少见情况），创建一个空的
  if (exitCode == 0 && !fs::exists(output_file))
  {
    std::ofstream empty(output_file, std::ios::binary);
  }

  try
  {
    if (fs::exists(lib_rsp_file)) fs::remove(lib_rsp_file);
  }
  catch (...)
  {
  }

  return exitCode;
}
