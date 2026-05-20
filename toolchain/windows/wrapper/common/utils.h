#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

// String conversion
std::string WStringToString(const std::wstring& wstr);
std::wstring StringToWString(const std::string& str);

// Filesystem helpers
bool directory_exists(const std::wstring& path);
void find_subdirectories(const std::wstring& parent_dir, std::vector<std::wstring>& candidates);

// PATH setup
void SetupSystemPath();

// PATH search
std::wstring FindExecutableInPath(const std::wstring& exeName, const std::wstring& pathVar);

// Environment map helpers
std::wstring GetEnvVarCaseInsensitive(const std::map<std::wstring, std::wstring>& envMap, const std::wstring& key);
std::vector<wchar_t> BuildEnvironmentBlock(const std::map<std::wstring, std::wstring>& envMap);
