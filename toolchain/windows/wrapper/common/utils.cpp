#include "common/utils.h"
#include <sstream>

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

bool directory_exists(const std::wstring& path)
{
  return fs::exists(path) && fs::is_directory(path);
}

void find_subdirectories(const std::wstring& parent_dir, std::vector<std::wstring>& candidates)
{
  if (!directory_exists(parent_dir))
  {
    return;
  }

  for (const auto& entry : fs::directory_iterator(parent_dir))
  {
    if (entry.is_directory())
    {
      candidates.push_back(entry.path().wstring());
    }
  }
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
      if (fs::exists(p)) return p.wstring();
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
