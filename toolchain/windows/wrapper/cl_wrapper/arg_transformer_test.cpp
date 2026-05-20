#include "cl_wrapper/arg_transformer.h"

#include <gtest/gtest.h>
#include <vector>
#include <string>

// =============================================================
// ProcessClArgument tests
// =============================================================

TEST(ProcessClArgument, PlainArg)
{
  EXPECT_EQ(ProcessClArgument("hello"), "hello");
}

TEST(ProcessClArgument, ArgWithSpaces)
{
  EXPECT_EQ(ProcessClArgument("hello world"), "\"hello world\"");
}

TEST(ProcessClArgument, AlreadyQuoted)
{
  EXPECT_EQ(ProcessClArgument("\"already quoted\""), "\"already quoted\"");
}

TEST(ProcessClArgument, LinkFlagNotQuoted)
{
  EXPECT_EQ(ProcessClArgument("/link"), "/link");
}

TEST(ProcessClArgument, DefineSimple)
{
  EXPECT_EQ(ProcessClArgument("/DFOO=bar"), "/DFOO=bar");
}

TEST(ProcessClArgument, DefineWithEmbeddedQuote)
{
  std::string result = ProcessClArgument("/DFOO=\"bar\"");
  EXPECT_NE(result.find("\\\""), std::string::npos);
}

// =============================================================
// TransformArgs: compilation mode
// =============================================================

TEST(TransformArgs, CompilationFlagC)
{
  std::vector<std::string> args = {"-c", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  EXPECT_TRUE(t.is_compilation);
}

TEST(TransformArgs, OutputFlagO)
{
  std::vector<std::string> args = {"-c", "-o", "out.obj", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  EXPECT_TRUE(t.is_compilation);
  EXPECT_EQ(t.output_file, "out.obj");
}

TEST(TransformArgs, OutputFoForCompile)
{
  std::vector<std::string> args = {"-c", "-o", "out.obj", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  bool has_fo = false;
  for (const auto& a : t.args) if (a == "/Foout.obj") has_fo = true;
  EXPECT_TRUE(has_fo);
}

TEST(TransformArgs, OutputFeForLink)
{
  std::vector<std::string> args = {"-o", "out.exe", "hello.obj"};
  TransformedArgs t = TransformArgs(args);
  EXPECT_FALSE(t.is_compilation);
  bool has_fe = false;
  for (const auto& a : t.args) if (a == "/Feout.exe") has_fe = true;
  EXPECT_TRUE(has_fe);
}

TEST(TransformArgs, IncludeI)
{
  std::vector<std::string> args = {"-c", "-Ipath/to/include", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  bool has_I = false;
  for (const auto& a : t.args) if (a == "/Ipath/to/include") has_I = true;
  EXPECT_TRUE(has_I);
}

TEST(TransformArgs, IncludeIsystem)
{
  std::vector<std::string> args = {"-c", "-isystem", "sys/path", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  bool has_sys = false;
  for (const auto& a : t.args) if (a == "/Isys/path") has_sys = true;
  EXPECT_TRUE(has_sys);
}

TEST(TransformArgs, IncludeIquote)
{
  std::vector<std::string> args = {"-c", "-iquote", "quote/path", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  bool has_quote = false;
  for (const auto& a : t.args) if (a == "/Iquote/path") has_quote = true;
  EXPECT_TRUE(has_quote);
}

TEST(TransformArgs, LibraryL)
{
  std::vector<std::string> args = {"-lfoo", "hello.obj"};
  TransformedArgs t = TransformArgs(args);
  bool has_lib = false;
  for (const auto& a : t.args) if (a == "foo.lib") has_lib = true;
  EXPECT_TRUE(has_lib);
}

TEST(TransformArgs, Defaultlib)
{
  std::vector<std::string> args = {"-defaultlib:foo.lib", "hello.obj"};
  TransformedArgs t = TransformArgs(args);
  bool has_lib = false;
  for (const auto& a : t.args) if (a == "foo.lib") has_lib = true;
  EXPECT_TRUE(has_lib);
}

TEST(TransformArgs, StdCpp17)
{
  std::vector<std::string> args = {"-std=c++17", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  bool has_std = false;
  for (const auto& a : t.args) if (a == "/std:c++17") has_std = true;
  EXPECT_TRUE(has_std);
}

TEST(TransformArgs, StdCpp20)
{
  std::vector<std::string> args = {"-std=c++20", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  bool has_std = false;
  for (const auto& a : t.args) if (a == "/std:c++20") has_std = true;
  EXPECT_TRUE(has_std);
}

TEST(TransformArgs, DepFlags)
{
  std::vector<std::string> args = {"-c", "-MF", "out.d", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  EXPECT_EQ(t.dep_file, "out.d");
}

TEST(TransformArgs, MDIgnored)
{
  std::vector<std::string> args = {"-MD", "-c", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  EXPECT_TRUE(t.is_compilation);
}

// =============================================================
// TransformArgs: standard flags
// =============================================================

TEST(TransformArgs, StandardCompileFlags)
{
  std::vector<std::string> args = {"-c", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  auto has = [&](const std::string& flag) {
    for (const auto& a : t.args) if (a == flag) return true;
    return false;
  };
  EXPECT_TRUE(has("/nologo"));
  EXPECT_TRUE(has("/EHsc"));
  EXPECT_TRUE(has("/c"));
  EXPECT_TRUE(has("/utf-8"));
}

TEST(TransformArgs, ShowIncludesForDepfile)
{
  std::vector<std::string> args = {"-c", "-MF", "out.d", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  auto has = [&](const std::string& flag) {
    for (const auto& a : t.args) if (a == flag) return true;
    return false;
  };
  EXPECT_TRUE(has("/showIncludes"));
}

TEST(TransformArgs, SubsystemForLink)
{
  std::vector<std::string> args = {"hello.obj"};
  TransformedArgs t = TransformArgs(args);
  auto has = [&](const std::string& flag) {
    for (const auto& a : t.args) if (a == flag) return true;
    return false;
  };
  EXPECT_TRUE(has("/SUBSYSTEM:CONSOLE"));
}

TEST(TransformArgs, DefaultRuntime)
{
  std::vector<std::string> args = {"-c", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  auto has = [&](const std::string& flag) {
    for (const auto& a : t.args) if (a == flag) return true;
    return false;
  };
  EXPECT_TRUE(has("/MD"));
}

TEST(TransformArgs, MDFlagNotDuplicated)
{
  std::vector<std::string> args = {"-c", "/MD", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  int md_count = 0;
  for (const auto& a : t.args) if (a == "/MD") md_count++;
  EXPECT_EQ(md_count, 1);
}

TEST(TransformArgs, FPICIgnored)
{
  std::vector<std::string> args = {"-c", "-fPIC", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  EXPECT_TRUE(t.is_compilation);
}

TEST(TransformArgs, FRandomSeedIgnored)
{
  std::vector<std::string> args = {"-c", "-frandom-seed=42", "hello.cpp"};
  TransformedArgs t = TransformArgs(args);
  EXPECT_TRUE(t.is_compilation);
}

TEST(TransformArgs, WlIgnored)
{
  std::vector<std::string> args = {"-Wl,-rpath,/foo", "hello.obj"};
  TransformedArgs t = TransformArgs(args);
  bool has_wl = false;
  for (const auto& a : t.args) if (a.find("-Wl,") == 0) has_wl = true;
  EXPECT_FALSE(has_wl);
}

TEST(TransformArgs, DedupObjLib)
{
  std::vector<std::string> args = {"foo.obj", "foo.obj", "bar.lib", "bar.lib"};
  TransformedArgs t = TransformArgs(args);
  int foo_count = 0, bar_count = 0;
  for (const auto& a : t.args)
  {
    if (a == "foo.obj") foo_count++;
    if (a == "bar.lib") bar_count++;
  }
  EXPECT_EQ(foo_count, 1);
  EXPECT_EQ(bar_count, 1);
}
