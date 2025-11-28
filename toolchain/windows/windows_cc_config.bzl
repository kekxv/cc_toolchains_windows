load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "action_config",
    "artifact_name_pattern",
    "feature",
    "flag_group",
    "flag_set",
    "tool",
    "tool_path",
    "variable_with_value",
)
load("@rules_cc//cc:defs.bzl", "CcToolchainConfigInfo")

def _impl(ctx):
    cpu = ctx.attr.cpu

    # 1. 确定架构和 Wrapper 后缀 (用于 cl/link)
    if cpu == "x64_windows":
        wrapper_suffix = ".bat"
        target_cpu_name = "x64_windows"
        arch_flag = "--arch=x64"
    elif cpu == "x86_windows":
        wrapper_suffix = "_x86.bat"
        target_cpu_name = "x86_windows"
        arch_flag = "--arch=x86"
    else:
        fail("Unsupported CPU: " + cpu)

    cl_wrapper = "cl_wrapper" + wrapper_suffix

    tool_paths = [
        tool_path(name = "gcc", path = cl_wrapper),
        tool_path(name = "cpp", path = cl_wrapper),
        tool_path(name = "ar", path = "lib_wrapper.exe"),
        tool_path(name = "ld", path = cl_wrapper),
        tool_path(name = "gcov", path = "false_wrapper.bat"),
        tool_path(name = "nm", path = "false_wrapper.bat"),
        tool_path(name = "objdump", path = "false_wrapper.bat"),
        tool_path(name = "strip", path = "false_wrapper.bat"),
    ]

    # =========================================================
    # Action Configs
    # =========================================================

    # A. 静态库打包
    cpp_link_static_library_action = action_config(
        action_name = ACTION_NAMES.cpp_link_static_library,
        tools = [tool(path = "lib_wrapper.exe")],
        implies = ["archiver_param_file"],
        flag_sets = [
            flag_set(
                flag_groups = [
                    # 1. 脚本路径 + 架构标志
                    flag_group(flags = [
                        arch_flag,
                    ]),
                    # 2. 操作符
                    flag_group(flags = ["rcsD"]),
                    # 3. 输出文件
                    flag_group(
                        expand_if_available = "output_execpath",
                        flags = ["%{output_execpath}"],
                    ),
                    # 4. 输入文件
                    flag_group(
                        iterate_over = "libraries_to_link",
                        flag_groups = [
                            flag_group(
                                expand_if_equal = variable_with_value(
                                    name = "libraries_to_link.type",
                                    value = "object_file",
                                ),
                                flags = ["%{libraries_to_link.name}"],
                            ),
                            flag_group(
                                expand_if_equal = variable_with_value(
                                    name = "libraries_to_link.type",
                                    value = "object_file_group",
                                ),
                                flags = ["%{libraries_to_link.object_files}"],
                            ),
                        ],
                    ),
                ],
            ),
        ],
    )

    # B. 可执行文件链接 (保持不变)
    cpp_link_executable_action = action_config(
        action_name = ACTION_NAMES.cpp_link_executable,
        tools = [tool(path = cl_wrapper)],
        implies = ["linker_param_file"],
        flag_sets = [
            flag_set(
                flag_groups = [
                    flag_group(
                        expand_if_available = "output_execpath",
                        flags = ["-o", "%{output_execpath}"],
                    ),
                    flag_group(
                        iterate_over = "libraries_to_link",
                        flag_groups = [
                            flag_group(
                                expand_if_equal = variable_with_value(
                                    name = "libraries_to_link.type",
                                    value = "object_file",
                                ),
                                flags = ["%{libraries_to_link.name}"],
                            ),
                            flag_group(
                                expand_if_equal = variable_with_value(
                                    name = "libraries_to_link.type",
                                    value = "object_file_group",
                                ),
                                flags = ["%{libraries_to_link.object_files}"],
                            ),
                            flag_group(
                                expand_if_equal = variable_with_value(
                                    name = "libraries_to_link.type",
                                    value = "static_library",
                                ),
                                flags = ["%{libraries_to_link.name}"],
                            ),
                            flag_group(
                                expand_if_equal = variable_with_value(
                                    name = "libraries_to_link.type",
                                    value = "interface_library",
                                ),
                                flags = ["%{libraries_to_link.name}"],
                            ),
                        ],
                    ),
                    flag_group(
                        iterate_over = "user_link_flags",
                        flags = ["%{user_link_flags}"],
                    ),
                ],
            ),
        ],
    )

    static_link_msvcrt_feature = feature(
        name = "static_link_msvcrt",
        enabled = False,
        flag_sets = [
            flag_set(
                actions = [ACTION_NAMES.cpp_compile, ACTION_NAMES.c_compile],
                flag_groups = [flag_group(flags = ["/MT"])],
            ),
        ],
    )

    archiver_param_file_feature = feature(name = "archiver_param_file", enabled = True)
    linker_param_file_feature = feature(name = "linker_param_file", enabled = True)

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        toolchain_identifier = "windows-msvc-" + cpu,
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = target_cpu_name,
        target_libc = "msvcrt",
        compiler = "msvc-cl",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
        action_configs = [
            cpp_link_static_library_action,
            cpp_link_executable_action,
        ],
        cxx_builtin_include_directories = [
            "C:/Program Files",
            "C:/Program Files (x86)",
            "C:/Windows",
        ],
        artifact_name_patterns = [
            artifact_name_pattern(category_name = "executable", prefix = "", extension = ".exe"),
            artifact_name_pattern(category_name = "static_library", prefix = "", extension = ".lib"),
            artifact_name_pattern(category_name = "dynamic_library", prefix = "", extension = ".dll"),
            artifact_name_pattern(category_name = "interface_library", prefix = "", extension = ".lib"),
            artifact_name_pattern(category_name = "object_file", prefix = "", extension = ".obj"),
        ],
        features = [
            static_link_msvcrt_feature,
            archiver_param_file_feature,
            linker_param_file_feature,
        ],
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {
        "cpu": attr.string(mandatory = True, values = ["x64_windows", "x86_windows"]),
    },
    provides = [CcToolchainConfigInfo],
)
