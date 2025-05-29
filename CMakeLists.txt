cmake_minimum_required(VERSION 3.13)
project(sdrmm)
option(OPT_OVERRIDE_STD_FILESYSTEM "Use a local version of std::filesystem on systems that don't have it yet" OFF)

set(SDRMM_MODULE_CMAKE "${CMAKE_SOURCE_DIR}/sdrmm_module.cmake")

set(SDRMM_CORE_ROOT "${CMAKE_SOURCE_DIR}/core/src/")

# Compiler flags
if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
    # Debug Flags
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(SDRMM_COMPILER_FLAGS -g -Og -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup)
    else ()
        set(SDRMM_COMPILER_FLAGS -g -Og -std=c++17)
    endif ()
else()
    # Normal Flags
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(SDRMM_COMPILER_FLAGS -O3 -std=c++17 -Wno-unused-command-line-argument -undefined dynamic_lookup)
    else ()
        set(SDRMM_COMPILER_FLAGS -O3 -std=c++17)
    endif ()
endif()
set(SDRMM_MODULE_COMPILER_FLAGS ${SDRMM_COMPILER_FLAGS})

# Set a default install prefix
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "..." FORCE)
endif()

# Core of SDR++
add_subdirectory("core")
add_executable(sdrmm "src/main.cpp")
add_subdirectory("source_modules/mirisdr_source")
target_link_libraries(sdrmm PRIVATE sdrmm_core)

# Compiler arguments
target_compile_options(sdrmm PRIVATE ${SDRMM_COMPILER_FLAGS})

add_custom_target(do_always ALL cp \"$<TARGET_FILE_DIR:sdrmm_core>/libsdrmm_core.so\" \"$<TARGET_FILE_DIR:sdrmm>\")

# Create module cmake file
configure_file(${CMAKE_SOURCE_DIR}/sdrmm_module.cmake ${CMAKE_CURRENT_BINARY_DIR}/sdrmm_module.cmake @ONLY)

# Install directives
install(TARGETS sdrmm DESTINATION bin)

# Create uninstall target
configure_file(${CMAKE_SOURCE_DIR}/cmake_uninstall.cmake ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake @ONLY)
add_custom_target(uninstall ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
