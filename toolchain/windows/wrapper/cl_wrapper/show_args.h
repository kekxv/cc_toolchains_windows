#pragma once

#include <string>
#include <map>
#include <vector>

// Handle /bazel-show-args=<name> debug flag.
// If present, prints --action_env stanzas to stdout and returns true.
// Returns false if no /bazel-show-args= flag found in argv.
bool HandleBazelShowArgs(int argc, char* argv[],
                         const std::map<std::wstring, std::wstring>& envMap);
