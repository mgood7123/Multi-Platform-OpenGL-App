# cmake -G Ninja -B build
# cmake --build build
# .\build\Windows-OS.exe

cmake_minimum_required(VERSION 3.8)
project("Windows-OS" C CXX)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(SKIA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../skia)

add_compile_definitions(NOMINMAX)

# If you are building Skia with the gn
#  build and have not set `is_official_build` you will get `is_trivial_abi`
#  set in the build which will end up marking `sk_sp` with `[[clang::trivial_abi]]`.
#
# If `SK_TRIVIAL_ABI` is not set consistently it will result in very
#  strange and difficult to understand issues depending on the platform
#  (since this is effectively changing the calling convention).

# /**
#  * Annotates a class' non-trivial special functions as trivial for the purposes of calls.
#  * Allows a class with a non-trivial destructor to be __is_trivially_relocatable.
#  * Use of this attribute on a public API breaks platform ABI.
#  * Annotated classes may not hold pointers derived from `this`.
#  * Annotated classes must implement move+delete as equivalent to memcpy+free.
#  * Use may require more complete types, as callee destroys.
#  *
#  * https://clang.llvm.org/docs/AttributeReference.html#trivial-abi
#  * https://libcxx.llvm.org/DesignDocs/UniquePtrTrivialAbi.html
#  */
# #if !defined(SK_TRIVIAL_ABI)
# #  define SK_TRIVIAL_ABI
# #endif

# for the reasons above, we set intentionally set is_trivial_abi to false

set(SKIA_ARGS "${SKIA_ARGS} is_trivial_abi=false") # see skia/include/private/base/SkAttributes.h

# a full debug build of skia takes over 8 GB in android, will probably take more on windows
# use a release build for now since it is significantly smaller
# and we have other platforms that we need to build for
set(SKIA_ARGS "${SKIA_ARGS} is_official_build=true")
set(SKIA_ARGS "${SKIA_ARGS} is_debug=false")

# we are building for windows clang-cl which ALWAYS uses clang, this would build a full debug build
# set(SKIA_ARGS "${SKIA_ARGS} extra_cflags=[ '-g3' ]")

set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_expat=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_harfbuzz=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_icu=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_libjpeg_turbo=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_libpng=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_libwebp=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_zlib=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_enable_tools=true")

message(STATUS "CMAKE_CXX_COMPILER = ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION = ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION_INTERNAL = ${CMAKE_CXX_COMPILER_VERSION_INTERNAL}")
message(STATUS "CMAKE_CACHE_MAJOR_VERSION = ${CMAKE_CACHE_MAJOR_VERSION}")

# CMAKE_CXX_COMPILER is an absolute path
#  CMAKE_CXX_COMPILER = C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Tools/Llvm/x64/bin/clang-cl.exe
#
#  we need to chop it at the end
#
string(LENGTH "${CMAKE_CXX_COMPILER}" SKIA_ARGS_CLANG_WIN_TMP_LENGTH)
string(LENGTH "/bin/clang-cl.exe" SKIA_ARGS_CLANG_WIN_TMP_LENGTH_UNWANTED)
math(EXPR SKIA_ARGS_CLANG_WIN_TMP_LENGTH_ADJUSTED "${SKIA_ARGS_CLANG_WIN_TMP_LENGTH} - ${SKIA_ARGS_CLANG_WIN_TMP_LENGTH_UNWANTED}" OUTPUT_FORMAT DECIMAL)
string(SUBSTRING "${CMAKE_CXX_COMPILER}" 0 ${SKIA_ARGS_CLANG_WIN_TMP_LENGTH_ADJUSTED} SKIA_ARGS_CLANG_WIN_TMP)
message(STATUS "LLVM DIR = ${SKIA_ARGS_CLANG_WIN_TMP}")
set(SKIA_ARGS "${SKIA_ARGS} clang_win=\"${SKIA_ARGS_CLANG_WIN_TMP}\"")

set(SKIA_ARGS "${SKIA_ARGS} clang_win_version=\"${CMAKE_CXX_COMPILER_VERSION}\"")

# SKIA_ARGS_CLANG_WIN_TMP is an absolute path
#  SKIA_ARGS_CLANG_WIN_TMP = C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Tools/Llvm/x64
#
#  we need to chop it at the end
#
string(LENGTH "${SKIA_ARGS_CLANG_WIN_TMP}" SKIA_ARGS_WIN_VC_TMP_LENGTH)
string(LENGTH "/Tools/Llvm/64" SKIA_ARGS_WIN_VC_TMP_LENGTH_UNWANTED)
# for some reason 'string(LENGTH "/Tools/Llvm/64") == string(LENGTH "Tools/Llvm/64")'
math(EXPR SKIA_ARGS_WIN_VC_TMP_LENGTH_ADJUSTED "${SKIA_ARGS_WIN_VC_TMP_LENGTH} - (${SKIA_ARGS_WIN_VC_TMP_LENGTH_UNWANTED}+1)" OUTPUT_FORMAT DECIMAL)
string(SUBSTRING "${SKIA_ARGS_CLANG_WIN_TMP}" 0 ${SKIA_ARGS_WIN_VC_TMP_LENGTH_ADJUSTED} SKIA_ARGS_WIN_VC_TMP)
message(STATUS "Visual Studio DIR = ${SKIA_ARGS_WIN_VC_TMP}")
set(SKIA_ARGS "${SKIA_ARGS} win_vc=\"${SKIA_ARGS_WIN_VC_TMP}\"")

include_directories("${SKIA_DIR}")

message(STATUS "SKIA_ARGS = ${SKIA_ARGS}")

message(STATUS "GN_OUTPUT_STDOUT.txt = ${CMAKE_CURRENT_SOURCE_DIR}/.CMAKE_TEMP_OUTPUT_GN_OUTPUT_STDOUT.txt")
message(STATUS "GN_OUTPUT_STDERR.txt = ${CMAKE_CURRENT_SOURCE_DIR}/.CMAKE_TEMP_OUTPUT_GN_OUTPUT_STDERR.txt")

execute_process(
        COMMAND
        "${SKIA_DIR}/bin/gn"
        "--root=${SKIA_DIR}"
        gen
        "${SKIA_DIR}/cmake_generated/windows/${CMAKE_BUILD_TYPE}"
        "--args=${SKIA_ARGS}"
        --ide=json
        "--json-ide-script=${SKIA_DIR}/gn/gn_to_cmake.py"
        OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.CMAKE_TEMP_OUTPUT_GN_OUTPUT_STDOUT.txt
        ERROR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.CMAKE_TEMP_OUTPUT_GN_OUTPUT_STDERR.txt
        COMMAND_ERROR_IS_FATAL ANY
)

#  a list of accepted arguments, useful to see what arguments we can tinker with
execute_process(
        COMMAND
        "${SKIA_DIR}/bin/gn"
        "--root=${SKIA_DIR}"
        args
        "${SKIA_DIR}/cmake_generated/windows/${CMAKE_BUILD_TYPE}"
        --list
        OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.CMAKE_TEMP_OUTPUT_GN_ARGS_OUTPUT_STDOUT.txt
        ERROR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.CMAKE_TEMP_OUTPUT_GN_ARGS_OUTPUT_STDERR.txt
        COMMAND_ERROR_IS_FATAL ANY
)

add_subdirectory(${SKIA_DIR}/cmake_generated/windows/${CMAKE_BUILD_TYPE} "C:/skia/${CMAKE_BUILD_TYPE}/windows")

add_executable(Windows-OS main.cpp win_msg.cpp)
target_link_libraries(Windows-OS PUBLIC skia tools__window)
set_property(TARGET Windows-OS PROPERTY CXX_STANDARD 17)
