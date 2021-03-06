cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_VERBOSE_MAKEFILE on)
set(libs "${CMAKE_SOURCE_DIR}/src/main/jniLibs")
include_directories(${CMAKE_SOURCE_DIR}/src/main/cpp)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/src/main/libs/${ANDROID_ABI})

add_library(libhiai SHARED IMPORTED )
set_target_properties(libhiai PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/src/main/jniLibs/${ANDROID_ABI}/libhiai.so")

add_library( # Sets the name of the library.
             hiaijni

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/mix_classify_jni.cpp
             src/main/cpp/mix_classify_async_jni.cpp
             src/main/cpp/mixbuildmodel.cpp)


# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       hiaijni android libhiai

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib} )