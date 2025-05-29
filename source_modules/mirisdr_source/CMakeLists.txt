cmake_minimum_required(VERSION 3.13)
project(mirisdr_source)

file(GLOB SRC "src/*.cpp")

include(${SDRMM_MODULE_CMAKE})

find_package(PkgConfig)

pkg_check_modules(LIBMIRISDR REQUIRED libmirisdr)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

target_include_directories(mirisdr_source PRIVATE ${LIBMIRISDR_INCLUDE_DIRS} ${LIBUSB_INCLUDE_DIRS})
target_link_directories(mirisdr_source PRIVATE ${LIBMIRISDR_LIBRARY_DIRS} ${LIBUSB_LIBRARY_DIRS})
target_link_libraries(mirisdr_source PRIVATE ${LIBMIRISDR_LIBRARIES} ${LIBUSB_LIBRARIES})