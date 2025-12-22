# cc_toolchains_windows

## install

- vs2019 or other version
- msys64
-

## use

```MODULE.bazel
bazel_dep(name = "cc_toolchains_windows", version = "0.0.1")
git_override(
    module_name = "cc_toolchains_windows",
    commit = "e6e877684895c90599ab3b6ca4b2a1197fd970b3",
    init_submodules = True,
    remote = "https://github.com/kekxv/cc_toolchains_windows.git",
)

register_execution_platforms(
    "@cc_toolchains_windows//:windows_x86",
    "@cc_toolchains_windows//:windows",
)

register_toolchains(
    "@cc_toolchains_windows//:windows_x86_toolchain",
    "@cc_toolchains_windows//:windows_toolchain",
)
```

## rules_foreign_cc

> disable check compiler work
> ``` 
> cache_entries = {
>     "CMAKE_C_COMPILER_WORKS": "TRUE",
>     "CMAKE_CXX_COMPILER_WORKS": "TRUE",
> },
> ```

[English](#english) | [中文](#chinese)

<a name="english"></a>

## English

A custom Bazel C++ toolchain for building on Windows using Microsoft Visual C++ (MSVC).

### Overview

This project provides a custom toolchain configuration that allows Bazel to use the locally installed Visual Studio
compiler (`cl.exe`) and linker (`lib.exe`) seamlessly. It bridges the gap between Bazel's expected interface and MSVC.

**Key Features:**

* **Auto-detection**: Automatically finds the latest Visual Studio installation using COM.
* **Dependency Management**: Captures `/showIncludes` output to generate `.d` files for correct incremental builds.
* **Argument Translation**: Translates GCC/Clang-style flags (e.g., `-o`, `-I`) to MSVC-compatible flags (e.g., `/Fe`,
  `/I`).
* **Environment Setup**: Automatically invokes `vcvarsall.bat` to set up the build environment.

### Prerequisites

* **Bazel**: Installed and available in your PATH.
* **Visual Studio**: A working installation of Visual Studio (2017, 2019, or 2022) with the "Desktop development with
  C++" workload.

### Usage

#### Configuration

The project includes pre-configured build options in `.bazelrc`:

* `--config=windows`: Builds for **64-bit** Windows (x64).
* `--config=windows86`: Builds for **32-bit** Windows (x86).

#### Building

To build the entire project:

```bash
# Build for x64 (Default/Recommended)
bazel build --config=windows //...

# Build for x86
bazel build --config=windows86 //...
```

#### Running Examples

To run the provided "Hello World" example:

```bash
bazel run --config=windows //examples/src:main
```

### Project Structure

* `toolchain/windows`: Contains the core toolchain definitions, wrappers (`cl_wrapper.exe`, `lib_wrapper.exe`), and
  configuration rules.
* `examples`: Sample C++ code (`lib` and `src`) to demonstrate usage.
* `platforms`: Platform definitions for x64 and x86.

---

<a name="chinese"></a>

## 中文 (Chinese)

用于在 Windows 上使用 Microsoft Visual C++ (MSVC) 进行构建的自定义 Bazel C++ 工具链。

### 简介

本项目提供了一套自定义的工具链配置，使 Bazel 能够无缝使用本地安装的 Visual Studio 编译器 (`cl.exe`) 和链接器 (`lib.exe`)
。它解决了 Bazel 标准接口与 MSVC 之间的差异。

**主要特性：**

* **自动检测**：使用 COM 接口自动查找最新的 Visual Studio 安装路径。
* **依赖管理**：捕获 `/showIncludes` 输出并生成 `.d` 文件，以支持正确的增量构建。
* **参数转换**：将 GCC/Clang 风格的标志（如 `-o`, `-I`）自动转换为 MSVC 兼容的标志（如 `/Fe`, `/I`）。
* **环境设置**：自动调用 `vcvarsall.bat` 设置构建环境。

### 前置要求

* **Bazel**：已安装并添加到系统 PATH 中。
* **Visual Studio**：已安装 Visual Studio (2017, 2019, 或 2022) 并勾选了“使用 C++ 的桌面开发”工作负载。

### 使用方法

#### 配置

项目在 `.bazelrc` 中预置了以下构建选项：

* `--config=windows`：构建 **64 位** Windows (x64) 程序。
* `--config=windows86`：构建 **32 位** Windows (x86) 程序。

#### 构建

构建整个项目：

```bash
# 构建 x64 版本 (默认/推荐)
bazel build --config=windows //...

# 构建 x86 版本
bazel build --config=windows86 //...
```

#### 运行示例

运行提供的 "Hello World" 示例：

```bash
bazel run --config=windows //examples/src:main
```

### 项目结构

* `toolchain/windows`：包含核心工具链定义、包装程序 (`cl_wrapper.exe`, `lib_wrapper.exe`) 和配置规则。
* `examples`：用于演示用法的 C++ 示例代码（包含 `lib` 和 `src`）。
* `platforms`：针对 x64 和 x86 构建的平台定义。
