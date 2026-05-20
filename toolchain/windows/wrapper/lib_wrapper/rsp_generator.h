#pragma once

#include <string>
#include <vector>
#include <iostream>

// Generate a UUID-based string for unique temporary filenames
std::string GetUUIDString();

// Parse lib.exe arguments: separates --arch=, /out:, and input files.
// Sets output_file (may be empty), populates input_files.
// Returns the target architecture string ("x64" or "x86").
std::string ParseLibArgs(const std::vector<std::string>& args,
                         std::string& output_file,
                         std::vector<std::string>& input_files);

// Generate a response file (.rsp) for lib.exe
void GenerateRspFile(const std::string& rsp_path,
                     const std::string& output_file,
                     const std::vector<std::string>& input_files);

// Infer output file name from param file path
std::string InferOutputFromParamFile(const std::string& param_file_path);
