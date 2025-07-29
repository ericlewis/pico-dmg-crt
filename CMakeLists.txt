# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_dmg_crt C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Add include directories
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories(${PICO_SDK_PATH}/src/common/pico_stdlib/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_pio/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_irq/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_clocks/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/hardware_vreg/include)
include_directories(${PICO_SDK_PATH}/src/rp2040/hardware_structs/include)
include_directories(${PICO_SDK_PATH}/src/rp2040/hardware_regs/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_multicore/include)
include_directories(${PICO_SDK_PATH}/src/rp2_common/pico_platform/include)

# Add executable first (before generating PIO headers)
add_executable(${PROJECT_NAME}
    src/main.c
    src/gb_capture.c
    src/video_output.c
)

# Generate PIO headers (after defining the target)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/gb_capture.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/hv_gen.pio)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/mono_stream3x.pio)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib 
    hardware_pio 
    hardware_dma 
    hardware_irq
    hardware_vreg
    pico_multicore
    pico_double
    pico_float
    m
)

# Enable stdio output only for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    pico_enable_stdio_uart(${PROJECT_NAME} 1)
    pico_enable_stdio_usb(${PROJECT_NAME} 0)
else()
    pico_enable_stdio_uart(${PROJECT_NAME} 0)
    pico_enable_stdio_usb(${PROJECT_NAME} 0)
endif()

# Optimization flags for release builds
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-O3>
    -Wall
    -Wextra
    -Wno-unused-parameter
)

# Disable features that require stdio for Release builds
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO_CXX_ENABLE_EXCEPTIONS=0
    $<$<CONFIG:Release>:PICO_PANIC_FUNCTION=custom_panic>
)

# Create map/bin/hex/uf2 file in addition to ELF  
pico_add_extra_outputs(${PROJECT_NAME})

# Place time-critical functions in RAM (moved after pico_add_extra_outputs)
pico_set_binary_type(${PROJECT_NAME} copy_to_ram)