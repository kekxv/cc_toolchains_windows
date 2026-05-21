#include "lib_wrapper/rsp_generator.h"

#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

// =============================================================
// GetUUIDString tests
// =============================================================

TEST(GetUUIDString, NonEmpty)
{
  std::string uuid = GetUUIDString();
  EXPECT_FALSE(uuid.empty());
}

TEST(GetUUIDString, Alphanumeric)
{
  std::string uuid = GetUUIDString();
  for (char c : uuid) EXPECT_TRUE(isalnum(c));
}

TEST(GetUUIDString, Unique)
{
  std::string u1 = GetUUIDString();
  std::string u2 = GetUUIDString();
  EXPECT_NE(u1, u2);
}

// =============================================================
// ParseLibArgs tests
// =============================================================

TEST(ParseLibArgs, DefaultArch)
{
  std::vector<std::string> args;
  std::string output;
  std::vector<std::string> inputs;
  std::string arch = ParseLibArgs(args, output, inputs);
  EXPECT_EQ(arch, "x64");
}

TEST(ParseLibArgs, ArchX86)
{
  std::vector<std::string> args = {"--arch=x86"};
  std::string output;
  std::vector<std::string> inputs;
  std::string arch = ParseLibArgs(args, output, inputs);
  EXPECT_EQ(arch, "x86");
}

TEST(ParseLibArgs, OutFlag)
{
  std::vector<std::string> args = {"/out:foo.lib", "bar.obj"};
  std::string output;
  std::vector<std::string> inputs;
  ParseLibArgs(args, output, inputs);
  EXPECT_EQ(output, "foo.lib");
}

TEST(ParseLibArgs, OutFlagCaseInsensitive)
{
  std::vector<std::string> args = {"/OUT:foo.lib", "bar.obj"};
  std::string output;
  std::vector<std::string> inputs;
  ParseLibArgs(args, output, inputs);
  EXPECT_EQ(output, "foo.lib");
}

TEST(ParseLibArgs, InputFiles)
{
  std::vector<std::string> args = {"/out:out.lib", "foo.obj", "bar.obj"};
  std::string output;
  std::vector<std::string> inputs;
  ParseLibArgs(args, output, inputs);
  EXPECT_EQ(inputs.size(), 2u);
  EXPECT_EQ(inputs[0], "foo.obj");
  EXPECT_EQ(inputs[1], "bar.obj");
}

TEST(ParseLibArgs, NologoIgnored)
{
  std::vector<std::string> args = {"/nologo", "foo.obj"};
  std::string output;
  std::vector<std::string> inputs;
  ParseLibArgs(args, output, inputs);
  EXPECT_EQ(inputs.size(), 1u);
  EXPECT_EQ(inputs[0], "foo.obj");
}

TEST(ParseLibArgs, MachineIgnored)
{
  std::vector<std::string> args = {"/machine:x64", "foo.obj"};
  std::string output;
  std::vector<std::string> inputs;
  ParseLibArgs(args, output, inputs);
  EXPECT_EQ(inputs.size(), 1u);
}

TEST(ParseLibArgs, ArFlagsIgnored)
{
  std::vector<std::string> args = {"rcs", "foo.obj"};
  std::string output;
  std::vector<std::string> inputs;
  ParseLibArgs(args, output, inputs);
  EXPECT_EQ(inputs.size(), 1u);
  EXPECT_EQ(inputs[0], "foo.obj");
}

TEST(ParseLibArgs, SlashInPathConverted)
{
  std::vector<std::string> args = {"/out:sub/dir/out.lib", "sub/dir/foo.obj"};
  std::string output;
  std::vector<std::string> inputs;
  ParseLibArgs(args, output, inputs);
  EXPECT_EQ(output, "sub\\dir\\out.lib");
  EXPECT_EQ(inputs[0], "sub\\dir\\foo.obj");
}

// =============================================================
// GenerateRspFile tests
// =============================================================

class RspFileTest : public ::testing::Test
{
protected:
  void TearDown() override
  {
    std::remove("test_lib.rsp");
    std::remove("test_space.rsp");
  }
};

TEST_F(RspFileTest, GenerateRsp)
{
  std::string rsp_path = "test_lib.rsp";
  std::string output = "out.lib";
  std::vector<std::string> inputs = {"foo.obj", "bar.obj"};

  GenerateRspFile(rsp_path, output, inputs);

  EXPECT_TRUE(fs::exists(rsp_path));

  std::ifstream ifs(rsp_path);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("/OUT:out.lib"), std::string::npos);
  EXPECT_NE(content.find("/NOLOGO"), std::string::npos);
  EXPECT_NE(content.find("/IGNORE:4006"), std::string::npos);
  EXPECT_NE(content.find("/IGNORE:4221"), std::string::npos);
  EXPECT_NE(content.find("foo.obj"), std::string::npos);
  EXPECT_NE(content.find("bar.obj"), std::string::npos);
}

TEST_F(RspFileTest, InputWithSpace)
{
  std::string rsp_path = "test_space.rsp";
  std::string output = "out.lib";
  std::vector<std::string> inputs = {"some dir/foo.obj"};

  GenerateRspFile(rsp_path, output, inputs);

  std::ifstream ifs(rsp_path);
  std::string content((std::istreambuf_iterator<char>(ifs)),
                       std::istreambuf_iterator<char>());

  EXPECT_NE(content.find("\"some dir/foo.obj\""), std::string::npos);
}

// =============================================================
// InferOutputFromParamFile tests
// =============================================================

TEST(InferOutputFromParamFile, FromParams)
{
  std::string result = InferOutputFromParamFile("C:\\tmp\\foo\\mylib.lib.params");
  EXPECT_NE(result.find("mylib.lib"), std::string::npos);
}

TEST(InferOutputFromParamFile, WithDashSuffix)
{
  std::string result = InferOutputFromParamFile("C:\\tmp\\foo\\mylib-2.params");
  EXPECT_NE(result.find("mylib"), std::string::npos);
}

TEST(InferOutputFromParamFile, NoParams)
{
  std::string result = InferOutputFromParamFile("not_a_param_file.txt");
  EXPECT_TRUE(result.empty());
}
