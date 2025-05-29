cmake_minimum_required(VERSION 3.13)
project(sdrmm_core)

# Main code
file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c")

add_definitions(-DSDRMM_IS_CORE)
if (MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

# Add code to dyn lib
add_library(sdrmm_core SHARED ${SRC} ${BACKEND_SRC})

# Set compiler options
target_compile_options(sdrmm_core PRIVATE ${SDRMM_COMPILER_FLAGS})

# Set the install prefix
target_compile_definitions(sdrmm_core PUBLIC INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

# Include core headers
target_include_directories(sdrmm_core PUBLIC "src/")
target_include_directories(sdrmm_core PUBLIC "src/imgui")

if (OPT_OVERRIDE_STD_FILESYSTEM)
    target_include_directories(sdrmm_core PUBLIC "std_replacement")
endif (OPT_OVERRIDE_STD_FILESYSTEM)

if (MSVC)
    # Volk
    target_link_libraries(sdrmm_core PUBLIC volk)

    # FFTW3
    find_package(FFTW3f CONFIG REQUIRED)
    target_link_libraries(sdrmm_core PUBLIC FFTW3::fftw3f)

    # WinSock2
    target_link_libraries(sdrmm_core PUBLIC wsock32 ws2_32 iphlpapi)

    # ZSTD
    find_package(zstd CONFIG REQUIRED)
    target_link_libraries(sdrmm_core PUBLIC zstd::libzstd_shared)
else()
    find_package(PkgConfig)

    pkg_check_modules(FFTW3 REQUIRED fftw3f)
    pkg_check_modules(VOLK REQUIRED volk)
    pkg_check_modules(LIBZSTD REQUIRED libzstd)

    target_include_directories(sdrmm_core PUBLIC
        ${VOLK_INCLUDE_DIRS}
        ${LIBZSTD_INCLUDE_DIRS}
        ${FFTW3_INCLUDE_DIRS}
    )
    
    target_link_directories(sdrmm_core PUBLIC
        ${FFTW3_LIBRARY_DIRS}
        ${VOLK_LIBRARY_DIRS}
        ${LIBZSTD_LIBRARY_DIRS}
        ${FFTW3_INCLUDE_DIRS}
    )

    target_link_libraries(sdrmm_core PUBLIC
        ${FFTW3_LIBRARIES}
        ${VOLK_LIBRARIES}
        ${LIBZSTD_LIBRARIES}
        ${FFTW3_INCLUDE_DIRS}
    )

    if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
        target_link_libraries(sdrmm_core PUBLIC stdc++fs)
    endif ()

endif ()

set(CORE_FILES ${RUNTIME_OUTPUT_DIRECTORY} PARENT_SCOPE)

# Install directives
install(TARGETS sdrmm_core DESTINATION lib)
