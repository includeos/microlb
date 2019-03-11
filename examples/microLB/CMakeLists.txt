cmake_minimum_required(VERSION 3.0)
# IncludeOS install location
project (micro_lb_example C CXX)

include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL RESULT_VARIABLE HAS_CONAN)
if (NOT HAS_CONAN)
  message(FATAL_ERROR "missing conanbuildinfo.cmake did you forget to run conan install ?")
endif()
conan_basic_setup()

include(os)

set(SOURCES
  service.cpp
)

os_add_executable(micro_lb_example "microLB Service" ${SOURCES})

# DRIVERS / PLUGINS:

if ("$ENV{PLATFORM}" STREQUAL "x86_solo5")
  os_add_drivers(micro_lb_example solo5net)
else()
  os_add_drivers(micro_lb_example
    virtionet     # Virtio networking
    vmxnet3
    boot_logger   # Display boot information
    # Use "boot --drivers ." to see other drivers
    # virtioblk # Virtio block device
    # ... Others from src/drivers
  )
endif()

os_add_stdout(micro_lb_example default_stdout)
