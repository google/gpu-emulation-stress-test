# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             native_entry_points

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/native_entry_points.cpp
             src/main/cpp/util.cpp
             src/main/cpp/matrix.cpp
             src/main/cpp/FileLoader.cpp
             src/main/cpp/lodepng.cpp
             src/main/cpp/TextureLoader.cpp
             src/main/cpp/OBJParse.cpp
             src/main/cpp/Entity.cpp
             src/main/cpp/RenderModel.cpp
             src/main/cpp/WorldState.cpp
             src/main/cpp/GLES2Renderer.cpp
             src/main/cpp/GLES3Renderer.cpp

             src/main/cpp/ActionCurve.cpp
             src/main/cpp/BezierCurve.cpp
             src/main/cpp/ParticleSystem.cpp
             src/main/cpp/ScopedProfiler.cpp

              )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native_entry_points

                       log
                       android

                       EGL
                       GLESv2
                       GLESv3)