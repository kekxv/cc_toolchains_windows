#include "common/process.h"
#include "common/utils.h"

#include <iostream>
#include <vector>

ProcessResult RunTool(
    const std::wstring& commandLine,
    const std::map<std::wstring, std::wstring>& envMap,
    LineHandler onLine)
{
  ProcessResult result = {1};

  HANDLE hStdOutRead, hStdOutWrite;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
  if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) return result;
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
    auto envCopy = envMap;

    wchar_t buf[32767];
    if (GetEnvVarCaseInsensitive(envCopy, L"SystemRoot").empty())
    {
      if (GetEnvironmentVariableW(L"SystemRoot", buf, 32767)) envCopy[L"SystemRoot"] = buf;
    }

    envBlock = BuildEnvironmentBlock(envCopy);
    pEnv = envBlock.data();
  }

  std::wstring cmdCopy = commandLine;
  BOOL success = CreateProcessW(
    NULL,
    &cmdCopy[0],
    NULL, NULL, TRUE,
    CREATE_UNICODE_ENVIRONMENT,
    pEnv,
    NULL,
    &si, &pi
  );

  CloseHandle(hStdOutWrite);

  if (!success)
  {
    std::cerr << "Failed to execute tool.\nCommand: " << WStringToString(commandLine)
              << "\nError: " << GetLastError() << std::endl;
    CloseHandle(hStdOutRead);
    return result;
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

      if (onLine(line))
      {
        std::cout << line << std::endl;
      }
    }
  }

  WaitForSingleObject(pi.hProcess, INFINITE);
  GetExitCodeProcess(pi.hProcess, &result.exitCode);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  CloseHandle(hStdOutRead);

  return result;
}
