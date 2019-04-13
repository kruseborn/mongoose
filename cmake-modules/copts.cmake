list(APPEND LLVM_FLAGS
    "-Wno-missing-braces"
    "-Wno-format-security"
    "-Wno-unused-variable"
)

list(APPEND MSVC_FLAGS
    "/W3"
    "/wd4005"
    "/wd4068"
    "/wd4180"
    "/wd4244"
    "/wd4267"
    "/wd4800"
    "/DNOMINMAX"
    "/DWIN32_LEAN_AND_MEAN"
    "/D_CRT_SECURE_NO_WARNINGS"
    "/D_SCL_SECURE_NO_WARNINGS"
)