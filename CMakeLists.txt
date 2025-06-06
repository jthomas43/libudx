cmake_minimum_required(VERSION 3.25)

find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

project(udx C)

fetch_package("github:libuv/libuv@1.48.0")

add_library(udx OBJECT)

set_target_properties(
  udx
  PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON
)

if(UNIX)
  target_compile_options(udx PRIVATE -Wall -Wextra)
endif()

if(WIN32)
  target_compile_options(udx PRIVATE /W4)
endif()

target_sources(
  udx
  INTERFACE
    include/udx.h
  PRIVATE
    src/cirbuf.h
    src/cirbuf.c
    src/endian.h
    src/endian.c
    src/queue.c
    src/queue.h
    src/link.h
    src/io.h
    src/udx.c
    src/udx_rate.c
    src/win_filter.h
    src/win_filter.c
)

target_include_directories(
  udx
  PUBLIC
    include
    $<TARGET_PROPERTY:uv,INTERFACE_INCLUDE_DIRECTORIES>
)

if(UNIX)
  target_sources(
    udx
    PRIVATE
      src/io_posix.c
  )
endif()

if(WIN32)
  target_sources(
    udx
    PRIVATE
      src/io_win.c
  )
endif()

add_library(udx_shared SHARED)

set_target_properties(
  udx_shared
  PROPERTIES
  OUTPUT_NAME udx
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(
  udx_shared
  PUBLIC
    udx
    uv
)

if(UNIX)
  target_link_libraries(
    udx_shared
    PRIVATE
      m
  )
endif()

add_library(udx_static STATIC)

set_target_properties(
  udx_static
  PROPERTIES
  OUTPUT_NAME udx
  PREFIX lib
)

target_link_libraries(
  udx_static
  PUBLIC
    udx
    uv_a
)

if(UNIX)
  target_link_libraries(
    udx_static
    PRIVATE
      m
  )
endif()

install(TARGETS udx_shared udx_static)

install(FILES include/udx.h DESTINATION include)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()

  add_subdirectory(test)
  add_subdirectory(examples)
endif()
