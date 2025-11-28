load("@rules_cc//cc:defs.bzl", "cc_toolchain")
load(":windows_cc_config.bzl", "cc_toolchain_config")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "toolchain_wrappers",
    srcs = glob([
        "*.bat",
        "*.exe",
    ]),
)

# 定义空文件占位
filegroup(name = "empty")

# ---------------------------------------------------------
# x64 配置
# ---------------------------------------------------------
cc_toolchain_config(
    name = "windows_x64_config",
    cpu = "x64_windows",
)

cc_toolchain(
    name = "cc-compiler-x64-windows",
    all_files = ":toolchain_wrappers",
    ar_files = ":toolchain_wrappers",
    as_files = ":empty",
    compiler_files = ":toolchain_wrappers",
    dwp_files = ":toolchain_wrappers",
    linker_files = ":toolchain_wrappers",
    objcopy_files = ":toolchain_wrappers",
    strip_files = ":toolchain_wrappers",
    supports_param_files = 1,
    toolchain_config = ":windows_x64_config",
    toolchain_identifier = "windows-msvc-x64_windows",
)

toolchain(
    name = "windows_x64_toolchain",
    exec_compatible_with = [
        "@platforms//os:windows",
        "@platforms//cpu:x86_64",
    ],
    target_compatible_with = [
        "@platforms//os:windows",
        "@platforms//cpu:x86_64",
    ],
    toolchain = ":cc-compiler-x64-windows",
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)

# ---------------------------------------------------------
# x86 配置
# ---------------------------------------------------------
cc_toolchain_config(
    name = "windows_x86_config",
    cpu = "x86_windows",
)

cc_toolchain(
    name = "cc-compiler-x86-windows",
    all_files = ":toolchain_wrappers",
    ar_files = ":toolchain_wrappers",
    as_files = ":empty",
    compiler_files = ":toolchain_wrappers",
    dwp_files = ":toolchain_wrappers",
    linker_files = ":toolchain_wrappers",
    objcopy_files = ":toolchain_wrappers",
    strip_files = ":toolchain_wrappers",
    supports_param_files = 1,
    toolchain_config = ":windows_x86_config",
    toolchain_identifier = "windows-msvc-x86_windows",
)

toolchain(
    name = "windows_x86_toolchain",
    exec_compatible_with = [
        "@platforms//os:windows",
        "@platforms//cpu:x86_64",
    ],
    target_compatible_with = [
        "@platforms//os:windows",
        "@platforms//cpu:x86_32",
    ],
    toolchain = ":cc-compiler-x86-windows",
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)
