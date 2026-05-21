#pragma once

#include <windows.h>
#include <string>
#include <map>

// Find Visual Studio installation path (COM API with filesystem fallback)
std::wstring FindVisualStudioPath();

// Find the latest MSVC toolchain version under VC/Tools/MSVC/
std::wstring FindLatestMSVCVersion(const std::wstring& vsPath);

// Load VS environment variables by running vcvars64.bat or vcvars32.bat.
// use32Bit: true = vcvars32.bat, false = vcvars64.bat
std::map<std::wstring, std::wstring> LoadVSEnvironment(const std::wstring& vsPath, bool use32Bit);
