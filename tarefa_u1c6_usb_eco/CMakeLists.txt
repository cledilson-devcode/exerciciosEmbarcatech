# CMakeLists.txt para Dispositivo CDC com TinyUSB

# Bloco da Extensão VSCode (ajuste as versões para sua instalação do SDK 2.1.1)
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion "2.1.1")
set(toolchainVersion "14_2_Rel1") # Verifique sua versão
set(picotoolVersion "2.1.1")   # Verifique sua versão
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Garante que o compilador entenda static_assert e outras extensões
if(CMAKE_C_FLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
else()
    set(CMAKE_C_FLAGS "-std=gnu11")
endif()

# PICO_BOARD é importante para o SDK configurar bibliotecas como TinyUSB corretamente.
# O log mostra que 'pico_w' está sendo usado, o que é bom.
set(PICO_BOARD pico_w CACHE STRING "Board type")


include(pico_sdk_import.cmake)

# O nome do projeto no seu log de erro é "dispositivo_cdc_visual"
# Vou usar esse nome aqui. Se o nome do seu projeto for outro, ajuste.
project(dispositivo_cdc_visual C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    tarefa_u1c6_usb_eco.c
)

# Configura stdio (USB será usado pelo TinyUSB CDC)
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Bibliotecas Linkadas
# O pico_stdlib, junto com pico_enable_stdio_usb(1),
# deve puxar as dependências necessárias do TinyUSB para CDC.
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    hardware_gpio  # Para controle dos LEDs
    # Removido pico_tinyusb_device daqui
)

pico_add_extra_outputs(${PROJECT_NAME})

pico_set_program_name(${PROJECT_NAME} "TAREFA_USB_ECO") # Use o nome do projeto ou um nome descritivo
pico_set_program_version(${PROJECT_NAME} "1.0")