#include "common/vs_env.h"
#include "common/com_defs.h"
#include "common/utils.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

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
        if (verStr > bestVerStr)
        {
          bestVerStr = verStr;
        }
      }
    }
  }
  return bestVerStr;
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
    std::vector<std::wstring> candidates;
    std::vector<std::wstring> base_paths;

    const wchar_t* program_files = _wgetenv(L"ProgramFiles");
    if (program_files) base_paths.push_back(std::wstring(program_files) + L"\\Microsoft Visual Studio");
    else
    {
      base_paths.emplace_back(L"C:\\Program Files\\Microsoft Visual Studio");
      base_paths.emplace_back(L"D:\\Program Files\\Microsoft Visual Studio");
      base_paths.emplace_back(L"E:\\Program Files\\Microsoft Visual Studio");
    }

    const wchar_t* program_files_x86 = _wgetenv(L"ProgramFiles(x86)");
    if (program_files_x86) base_paths.push_back(std::wstring(program_files_x86) + L"\\Microsoft Visual Studio");
    else
    {
      base_paths.emplace_back(L"C:\\Program Files (x86)\\Microsoft Visual Studio");
      base_paths.emplace_back(L"D:\\Program Files (x86)\\Microsoft Visual Studio");
      base_paths.emplace_back(L"E:\\Program Files (x86)\\Microsoft Visual Studio");
    }

    std::vector<std::wstring> year_dirs;
    for (const auto& base_path : base_paths) find_subdirectories(base_path, year_dirs);

    for (const auto& year_dir : year_dirs) find_subdirectories(year_dir, candidates);

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

std::map<std::wstring, std::wstring> LoadVSEnvironment(const std::wstring& vsPath, bool use32Bit)
{
  std::map<std::wstring, std::wstring> envMap;

  std::wstring scriptName = use32Bit ? L"vcvars32.bat" : L"vcvars64.bat";

  std::wstring path1 = vsPath + L"\\VC\\Auxiliary\\Build\\" + scriptName;
  std::wstring path2 = vsPath + L"\\Auxiliary\\Build\\" + scriptName;

  std::wstring vcvars;
  if (fs::exists(path1)) vcvars = path1;
  else if (fs::exists(path2)) vcvars = path2;
  else return envMap;

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
