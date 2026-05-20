#include "cl_wrapper/depfile.h"

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// =============================================================
// ParseShowIncludes tests
// =============================================================

TEST(ParseShowIncludes, EnglishInclude)
{
  std::vector<std::string> includes;
  bool consumed = ParseShowIncludes("Note: including file:  C:\\Program Files\\foo\\bar.h", includes);
  EXPECT_TRUE(consumed);
  EXPECT_GT(includes.size(), 0u);
}

TEST(ParseShowIncludes, ChineseInclude)
{
  std::vector<std::string> includes;
  bool consumed = ParseShowIncludes("注意: 包含文件:  C:\\Program Files\\foo\\bar.h", includes);
  EXPECT_TRUE(consumed);
  EXPECT_GT(includes.size(), 0u);
}

TEST(ParseShowIncludes, NonIncludeLine)
{
  std::vector<std::string> includes;
  bool consumed = ParseShowIncludes("hello.cpp", includes);
  EXPECT_FALSE(consumed);
  EXPECT_TRUE(includes.empty());
}

TEST(ParseShowIncludes, OrdinaryOutput)
{
  std::vector<std::string> includes;
  bool consumed = ParseShowIncludes("   Creating library foo.lib", includes);
  EXPECT_FALSE(consumed);
  EXPECT_TRUE(includes.empty());
}

// =============================================================
// WriteDepFile tests
// =============================================================

class DepFileTest : public ::testing::Test
{
protected:
  void TearDown() override
  {
    std::remove("test_output.d");
    std::remove("test_empty.d");
    std::remove("test_space.d");
  }
};

TEST_F(DepFileTest, WriteDepFile)
{
  std::string dep_path = "test_output.d";
  std::string output = "out.obj";
  std::vector<std::string> includes = {
    "C:/Program Files/foo/bar.h",
    "C:/Users/test/src/helper.h"
  };

  WriteDepFile(dep_path, output, includes);

  EXPECT_TRUE(fs::exists(dep_path));

  std::ifstream ifs(dep_path);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("out.obj"), std::string::npos);
  EXPECT_NE(content.find("foo/bar.h"), std::string::npos);
  EXPECT_NE(content.find("helper.h"), std::string::npos);
  EXPECT_NE(content.find(":"), std::string::npos);
  EXPECT_NE(content.find("\\\n"), std::string::npos);
}

TEST_F(DepFileTest, EmptyIncludes)
{
  std::string dep_path = "test_empty.d";
  std::string output = "out.obj";
  std::vector<std::string> includes;

  WriteDepFile(dep_path, output, includes);

  EXPECT_TRUE(fs::exists(dep_path));

  std::ifstream ifs(dep_path);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("out.obj"), std::string::npos);
  EXPECT_NE(content.find(":"), std::string::npos);
}

TEST_F(DepFileTest, SpacesEscaped)
{
  std::string dep_path = "test_space.d";
  std::string output = "out file.obj";
  std::vector<std::string> includes = {"C:/path/with spaces/header.h"};

  WriteDepFile(dep_path, output, includes);

  std::ifstream ifs(dep_path);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());

  // output filename should have spaces escaped with backslash-space
  EXPECT_NE(content.find("out\\ file.obj"), std::string::npos);
}
