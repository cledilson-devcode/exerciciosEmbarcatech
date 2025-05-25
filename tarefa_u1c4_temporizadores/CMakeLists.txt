# CMakeLists.txt para Semáforo de Trânsito Interativo

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

set(PICO_BOARD pico CACHE STRING "Board type") # Ou 'pico_w' se estiver usando um Pico W

include(pico_sdk_import.cmake)

project(semaforo_interativo C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT_NAME}
    tarefaU1C4Temporizadores.c
)

# Configura stdio
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Bibliotecas Linkadas
target_link_libraries(${PROJECT_NAME} PRIVATE
    hardware_irq
    pico_sync
    pico_stdlib
    hardware_gpio  # Para controle dos LEDs e leitura do botão
    hardware_timer # Essencial para alarmes e temporizadores
)

pico_add_extra_outputs(${PROJECT_NAME})

pico_set_program_name(${PROJECT_NAME} "SemaforoInterativo")
pico_set_program_version(${PROJECT_NAME} "1.0")