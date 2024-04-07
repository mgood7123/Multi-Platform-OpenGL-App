# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("AndroidGame")

# Creates your game shared library. The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(AndroidGame SHARED
        host/main.cpp
        host/SkiaRenderer.cpp
        host/skia_app.cpp
)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

set(SKIA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../skia")

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

# a full debug build of skia takes over 8 GB
# use a release build for now since it is significantly smaller
# and we have other platforms that we need to build for
set(SKIA_ARGS "${SKIA_ARGS} is_official_build=true")
set(SKIA_ARGS "${SKIA_ARGS} is_debug=false")

# we are building for android which ALWAYS uses clang, this would build a full debug build
# set(SKIA_ARGS "${SKIA_ARGS} extra_cflags=[ '-g3' ]")

set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_expat=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_harfbuzz=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_icu=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_libjpeg_turbo=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_libpng=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_libwebp=false")
set(SKIA_ARGS "${SKIA_ARGS} skia_use_system_zlib=false")
set(SKIA_ARGS "${SKIA_ARGS} target_cpu=\"${CMAKE_ANDROID_ARCH}\"")

if (${CMAKE_HOST_WIN32})
    # windows uses NTFS convention of DRIVE_LETTER:/path_to_dir
    # skia does not like the NTFS drive letter in ndk argument
    # so we need to chop off 'C:' ( 'C:/foo' => '/foo' )
    string(LENGTH "${CMAKE_ANDROID_NDK}" SKIA_ARGS_NDK_TMP_LENGTH)
    string(SUBSTRING "${CMAKE_ANDROID_NDK}" 2 ${SKIA_ARGS_NDK_TMP_LENGTH} SKIA_ARGS_NDK_TMP)
    set(SKIA_ARGS "${SKIA_ARGS} ndk=\"${SKIA_ARGS_NDK_TMP}\"")
else()
    # all other platforms use unix convention of /path_to_dir
    set(SKIA_ARGS "${SKIA_ARGS} ndk=\"${CMAKE_ANDROID_NDK}\"")
endif()

set(SKIA_ARGS "${SKIA_ARGS} ndk_api=${ANDROID_NATIVE_API_LEVEL}")

include_directories("${SKIA_DIR}")

message(STATUS "GN_OUTPUT_STDOUT.txt = ${CMAKE_CURRENT_SOURCE_DIR}/.CMAKE_TEMP_OUTPUT_GN_OUTPUT_STDOUT.txt")
message(STATUS "GN_OUTPUT_STDERR.txt = ${CMAKE_CURRENT_SOURCE_DIR}/.CMAKE_TEMP_OUTPUT_GN_OUTPUT_STDERR.txt")

execute_process(
        COMMAND
        "${SKIA_DIR}/bin/gn"
        "--root=${SKIA_DIR}"
        gen
        "${SKIA_DIR}/cmake_generated/android/${CMAKE_BUILD_TYPE}"
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
        "${SKIA_DIR}/cmake_generated/android/${CMAKE_BUILD_TYPE}"
        --list
        OUTPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.CMAKE_TEMP_OUTPUT_GN_ARGS_OUTPUT_STDOUT.txt
        ERROR_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.CMAKE_TEMP_OUTPUT_GN_ARGS_OUTPUT_STDERR.txt
        COMMAND_ERROR_IS_FATAL ANY
)

add_subdirectory(${SKIA_DIR}/cmake_generated/android/${CMAKE_BUILD_TYPE} "C:/skia/${CMAKE_BUILD_TYPE}/android")

# Configure libraries CMake uses to link your target library.
target_link_libraries(AndroidGame
        # The game activity
        game-activity::game-activity_static
        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        EGL
        GLESv3
        jnigraphics
        android
        log
        skia tools__window
)
