cmake_minimum_required(VERSION 3.23)

project(upd-detector LANGUAGES C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Require out-of-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source build not allowed!")
endif()

# Include modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Flags
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(WARNINGS_AS_ERRORS ON CACHE BOOL "Treat warnings as errors" FORCE)

add_compile_definitions(_GNU_SOURCE)

# Separate internal C flags
separate_arguments(INTERN_C_FLAGS)

# Subdirectories
add_subdirectory(src)

