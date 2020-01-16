cmake_minimum_required(VERSION 3.6)

project(animatch C)

SET(LIBSUPERDERPY_GAMENAME "animatch" CACHE INTERNAL "")
SET(LIBSUPERDERPY_GAMENAME_PRETTY "Animatch" CACHE INTERNAL "")

SET(LIBSUPERDERPY_VENDOR "Holy Pangolin")
SET(LIBSUPERDERPY_APPID "com.holypangolin.Animatch")
SET(LIBSUPERDERPY_ORIENTATION "PORTRAIT")
SET(LIBSUPERDERPY_VERSION "1.0.2")

set(LIBSUPERDERPY_IMGUI ON CACHE INTERNAL "")

set(EMSCRIPTEN_TOTAL_MEMORY "512" CACHE INTERNAL "")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_SOURCE_DIR}/libsuperderpy/cmake")

include(libsuperderpy)

add_subdirectory(libsuperderpy)
add_subdirectory(src)
add_subdirectory(data)
