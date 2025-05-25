# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

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
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(picow_access_point_background C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(picow_access_point_background main.c 
    dhcpserver/dhcpserver.c 
    dnsserver/dnsserver.c 
    src/buzzer.c 

    src/display/display_app.c 
    src/display/ssd1306_init.c
    src/display/ssd1306_text.c
    src/display/ssd1306_utils.c
    src/display/oled_setup.c
    src/display/oled_messages.c



)

pico_set_program_name(picow_access_point_background "picow_access_point_background")
pico_set_program_version(picow_access_point_background "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(picow_access_point_background 0)
pico_enable_stdio_usb(picow_access_point_background 1)

# Add the standard library to the build
target_link_libraries(picow_access_point_background 
    pico_cyw43_arch_lwip_threadsafe_background 
    pico_stdlib 
    hardware_pwm 
    hardware_clocks
    hardware_i2c
)

# Add the standard include files to the build
target_include_directories(picow_access_point_background PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/dnsserver

        ${CMAKE_CURRENT_LIST_DIR}/inc
        ${CMAKE_CURRENT_LIST_DIR}/src

        ${CMAKE_CURRENT_LIST_DIR}/inc/display
        ${CMAKE_CURRENT_LIST_DIR}/src/display

        
        )

# Add any user requested libraries
#target_link_libraries(picow_access_point_background)

pico_add_extra_outputs(picow_access_point_background)

# You can change the address below to change the address of the access point
    pico_configure_ip4_address(picow_access_point_background PRIVATE
    CYW43_DEFAULT_IP_AP_ADDRESS 192.168.4.1
        )