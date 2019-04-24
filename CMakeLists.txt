cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(microlb C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT ARCH)
  set(ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

add_definitions(-DARCH_${ARCH})
add_definitions(-DARCH="${ARCH}")

set(TRIPLE "${ARCH}-pc-linux-elf")
set(CMAKE_CXX_COMPILER_TARGET ${TRIPLE})
set(CMAKE_C_COMPILER_TARGET ${TRIPLE})

option(LIVEUPDATE "Enable liveupdate" ON)
option(TLS "Enable secure connections" ON)

if (EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
else()
  if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
  endif()
  if (CONAN_PROFILE)
    set(CONANPROFILE PROFILE ${CONAN_PROFILE})
  endif()
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
     message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
     file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.13/conan.cmake"
                   "${CMAKE_BINARY_DIR}/conan.cmake")
  endif()
  include(${CMAKE_BINARY_DIR}/conan.cmake)
  conan_cmake_run(
    CONANFILE conanfile.py
    BASIC_SETUP
    ${CONANPROFILE}
  )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


set(LIBRARY_SRCS
  src/autoconf.cpp
  src/balancer.cpp
  src/defaults.cpp
  src/node.cpp
  src/nodes.cpp
  src/session.cpp
)

if (TLS)
  list(APPEND LIBRARY_SRCS
    src/openssl.cpp
  )
endif()


if (LIVEUPDATE)
  list(APPEND LIBRARY_SRCS
    src/serialize.cpp
  )
endif()

set(HDRS
  include/microLB
  include/balancer.hpp
  include/node.hpp
  include/nodes.hpp
  include/session.hpp
)

# microLB static library
add_library(microlb STATIC ${LIBRARY_SRCS})

if (LIVEUPDATE)
  set_target_properties(microlb PROPERTIES COMPILE_DEFINITIONS "LIVEUPDATE")
endif()

set_target_properties(microlb PROPERTIES PUBLIC_HEADER "${HDRS}")

install(TARGETS microlb
        ARCHIVE DESTINATION lib
        PUBLIC_HEADER DESTINATION include
)
