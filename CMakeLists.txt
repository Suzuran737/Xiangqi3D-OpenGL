cmake_minimum_required(VERSION 3.20)

project(Xiangqi3D VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output folders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# OpenGL (system)
find_package(OpenGL REQUIRED)

option(XIANGQI3D_USE_SYSTEM_DEPS "Use system-installed dependencies instead of local third_party" OFF)
option(XIANGQI3D_USE_LOCAL_GLAD "Use local glad under third_party/glad instead of find_package(glad)" ON)

set(GLFW_TARGET "")
set(GLAD_TARGET "")
set(GLM_TARGET "glm::glm")
set(ASSIMP_TARGET "assimp")
set(FREETYPE_TARGET "freetype")

# ---- Local GLAD (your copied files) ----
function(xiangqi3d_setup_local_glad out_target)
  set(_glad_include_dir "${CMAKE_SOURCE_DIR}/third_party/glad/include")

  # Support both common filenames:
  # glad2: src/gl.c + include/glad/gl.h
  # glad1: src/glad.c + include/glad/glad.h (you can also provide a wrapper glad/gl.h)
  set(_glad_src "")
  if (EXISTS "${CMAKE_SOURCE_DIR}/third_party/glad/src/gl.c")
    set(_glad_src "${CMAKE_SOURCE_DIR}/third_party/glad/src/gl.c")
  elseif (EXISTS "${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c")
    set(_glad_src "${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c")
  endif()

  if (_glad_src STREQUAL "")
    message(FATAL_ERROR
      "Local GLAD enabled, but glad source not found. Put gl.c or glad.c at: third_party/glad/src/")
  endif()

  if (NOT EXISTS "${_glad_include_dir}")
    message(FATAL_ERROR
      "Local GLAD enabled, but include dir not found: third_party/glad/include")
  endif()

  add_library(glad STATIC "${_glad_src}")
  target_include_directories(glad PUBLIC "${_glad_include_dir}")

  # Your project includes <glad/gl.h>. Make sure this exists:
  # third_party/glad/include/glad/gl.h
  # If you only have glad/glad.h, create a wrapper:
  # third_party/glad/include/glad/gl.h -> includes glad/glad.h

  set(${out_target} glad PARENT_SCOPE)
endfunction()

if (XIANGQI3D_USE_SYSTEM_DEPS)
  # Expect deps installed via MSYS2 pacman, vcpkg, etc.
  find_package(glfw3 CONFIG REQUIRED)
  find_package(glm CONFIG REQUIRED)
  find_package(assimp CONFIG REQUIRED)
  find_package(Freetype REQUIRED)

  # GLFW target name varies across distros.
  if (TARGET glfw)
    set(GLFW_TARGET glfw)
  elseif (TARGET glfw3)
    set(GLFW_TARGET glfw3)
  elseif (TARGET glfw::glfw)
    set(GLFW_TARGET glfw::glfw)
  else()
    message(FATAL_ERROR "Could not find a GLFW CMake target after find_package(glfw3).")
  endif()

  if (TARGET assimp::assimp)
    set(ASSIMP_TARGET assimp::assimp)
  endif()

  if (TARGET freetype::freetype)
    set(FREETYPE_TARGET freetype::freetype)
  elseif (TARGET Freetype::Freetype)
    set(FREETYPE_TARGET Freetype::Freetype)
  endif()

  if (XIANGQI3D_USE_LOCAL_GLAD)
    xiangqi3d_setup_local_glad(GLAD_TARGET)
  else()
    find_package(glad CONFIG REQUIRED)
    if (TARGET glad::glad)
      set(GLAD_TARGET glad::glad)
    elseif (TARGET glad)
      set(GLAD_TARGET glad)
    elseif (TARGET glad_gl)
      set(GLAD_TARGET glad_gl)
    else()
      message(FATAL_ERROR "Could not find a GLAD CMake target after find_package(glad).")
    endif()
  endif()

else()
  # ---- Local third_party mode ----
  if (NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/glfw/CMakeLists.txt")
    message(FATAL_ERROR "Local deps enabled, but third_party/glfw not found.")
  endif()
  if (NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/glm/CMakeLists.txt")
    message(FATAL_ERROR "Local deps enabled, but third_party/glm not found.")
  endif()
  if (NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/assimp/CMakeLists.txt")
    message(FATAL_ERROR "Local deps enabled, but third_party/assimp not found.")
  endif()
  if (NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/freetype/CMakeLists.txt")
    message(FATAL_ERROR "Local deps enabled, but third_party/freetype not found.")
  endif()

  # ---- GLFW ----
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glfw)

  # ---- GLM ----
  set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
  add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glm)

  # ---- Assimp ----
  set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
  set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
  add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/assimp)

  # ---- FreeType ----
  set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
  set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
  set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
  set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
  add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/freetype)

  set(GLFW_TARGET glfw)
  set(GLM_TARGET glm::glm)
  set(ASSIMP_TARGET assimp)
  if (TARGET assimp::assimp)
    set(ASSIMP_TARGET assimp::assimp)
  endif()

  set(FREETYPE_TARGET freetype)
  if (TARGET freetype::freetype)
    set(FREETYPE_TARGET freetype::freetype)
  endif()

  if (XIANGQI3D_USE_LOCAL_GLAD)
    xiangqi3d_setup_local_glad(GLAD_TARGET)
  else()
    message(FATAL_ERROR "Local deps mode without local glad is not supported. Enable XIANGQI3D_USE_LOCAL_GLAD=ON.")
  endif()
endif()

# ---- Executable ----
file(GLOB_RECURSE XIANGQI_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.cpp
  ${CMAKE_SOURCE_DIR}/include/*.h
  ${CMAKE_SOURCE_DIR}/include/*.hpp
)

add_executable(Xiangqi3D ${XIANGQI_SOURCES})

target_include_directories(Xiangqi3D PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/third_party
)

# GLFW should not include legacy GL headers
target_compile_definitions(Xiangqi3D PRIVATE GLFW_INCLUDE_NONE)

# Warnings
if (MSVC)
  target_compile_options(Xiangqi3D PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(Xiangqi3D PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Link libraries
target_link_libraries(Xiangqi3D PRIVATE
  OpenGL::GL
  ${GLFW_TARGET}
  ${GLAD_TARGET}
  ${GLM_TARGET}
  ${ASSIMP_TARGET}
  ${FREETYPE_TARGET}
)

# Copy assets next to the executable after build
add_custom_command(TARGET Xiangqi3D POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:Xiangqi3D>/assets
)
