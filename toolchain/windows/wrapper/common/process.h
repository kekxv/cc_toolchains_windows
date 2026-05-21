#pragma once

#include <windows.h>
#include <string>
#include <map>
#include <vector>
#include <functional>

struct ProcessResult
{
  DWORD exitCode;
};

// Line callback: receives each output line; return true to print to stdout, false to suppress
using LineHandler = std::function<bool(const std::string& line)>;

// Execute a tool with optional MSVC environment block.
// Calls onLine for each line of stdout; lines are printed only if onLine returns true.
ProcessResult RunTool(
    const std::wstring& commandLine,
    const std::map<std::wstring, std::wstring>& envMap,
    LineHandler onLine);
