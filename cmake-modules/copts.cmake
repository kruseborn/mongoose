list(APPEND LLVM_FLAGS
    "-Wno-gnu-anonymous-struct"
    "-Wno-nested-anon-types"
    "-mavx"
    "-Wall"
    "-Werror"
)

list(APPEND MSVC_FLAGS
    "/W3"
    "/WX"
     "/DNOMINMAX"
    "/DWIN32_LEAN_AND_MEAN"
    "/D_CRT_SECURE_NO_WARNINGS"
    "/D_SCL_SECURE_NO_WARNINGS"
    "/D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS"
	  "/std:c++latest"
)

macro(fix_default_compiler_settings_)
  if (MSVC)
    # For MSVC, CMake sets certain flags to defaults we want to override.
    # This replacement code is taken from sample in the CMake Wiki at
    # https://gitlab.kitware.com/cmake/community/wikis/FAQ#dynamic-replace.
    foreach (flag_var
             CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
             CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
             CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
             CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      # Replaces /W3 with "" in defaults.
      string(REPLACE "/W3" "" ${flag_var} "${${flag_var}}")
    endforeach()
  endif()
endmacro()