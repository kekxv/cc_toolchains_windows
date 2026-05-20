#pragma once

#include <string>
#include <vector>

// Parse cl.exe /showIncludes output to collect dependency paths.
// Returns true if line is an include note (and was consumed), false otherwise.
bool ParseShowIncludes(const std::string& line, std::vector<std::string>& includes);

// Write a Make-style dependency file (.d)
void WriteDepFile(const std::string& dep_file, const std::string& output_file,
                  const std::vector<std::string>& includes);
