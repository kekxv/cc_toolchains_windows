#include "cl_wrapper/depfile.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace fs = std::filesystem;

bool ParseShowIncludes(const std::string& line, std::vector<std::string>& includes)
{
  if (line.find("including file:") == std::string::npos &&
      line.find("包含文件:") == std::string::npos)
    return false;

  // 路径在 "file:" (英文) 或 "文件:" (中文) 之后
  // "文件:" 在 UTF-8 中占 7 字节 (文=3, 件=3, :=1)
  size_t fileMarker = line.find("file:");
  size_t skipLen = 5;
  if (fileMarker == std::string::npos)
  {
    fileMarker = line.find("文件:");
    skipLen = 7;
  }
  if (fileMarker == std::string::npos)
    return true; // 无法解析路径，但已消费该行（不打印到 stdout）

  std::string pathStr = line.substr(fileMarker + skipLen);
  size_t firstNonSpace = pathStr.find_first_not_of(" ");
  if (firstNonSpace != std::string::npos) pathStr = pathStr.substr(firstNonSpace);

  try
  {
    fs::path absP = fs::absolute(pathStr);
    fs::path relP = fs::relative(absP, fs::current_path());
    if (relP.string().find("..") == 0 || relP.string().find(':') != std::string::npos)
    {
      includes.push_back(absP.string());
    }
    else
    {
      std::string relStr = relP.string();
      std::transform(relStr.begin(), relStr.end(), relStr.begin(), ::tolower);
      includes.push_back(relStr);
    }
  }
  catch (...)
  {
    includes.push_back(pathStr);
  }
  return true;
}

void WriteDepFile(const std::string& dep_file, const std::string& output_file,
                  const std::vector<std::string>& includes)
{
  std::ofstream dofs(dep_file);
  if (!dofs.is_open()) return;

  std::string out = output_file;
  std::replace(out.begin(), out.end(), '\\', '/');
  std::string esc_out;
  for (char c : out)
  {
    if (c == ' ') esc_out += "\\ ";
    else esc_out += c;
  }

  dofs << esc_out << ":";

  for (auto inc : includes)
  {
    std::replace(inc.begin(), inc.end(), '\\', '/');
    std::string esc_inc;
    for (char c : inc)
    {
      if (c == ' ') esc_inc += "\\ ";
      else esc_inc += c;
    }
    dofs << " \\\n  " << esc_inc;
  }
  dofs << "\n";
}
