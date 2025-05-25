# CMakeLists.txt para o projeto captar_som (com arquivo fonte captar_som.c)

# Bloco da Extensão VSCode (ajustado para SDK 2.1.1)
# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
# Defina aqui as versões que correspondem à sua instalação do SDK 2.1.1
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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Boa prática

# Configuração para reconhecer static_assert etc.
if(CMAKE_C_FLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
else()
    set(CMAKE_C_FLAGS "-std=gnu11")
endif()

set(PICO_BOARD pico CACHE STRING "Board type") # 'pico' ou 'pico_w'

include(pico_sdk_import.cmake) # Inclui o pico_sdk_import.cmake padrão

project(captar_som C CXX ASM) # Nome do projeto

pico_sdk_init()

# Adiciona o executável usando ${PROJECT_NAME} (captar_som)
# e o NOVO nome do arquivo .c principal.
add_executable(${PROJECT_NAME}
        captar_som.c  # <--- NOME DO ARQUIVO .C ATUALIZADO AQUI
        # neopixel.c NÃO é listado aqui porque é incluído diretamente em captar_som.c
)

# Define o nome e versão do programa (visível com picotool)
pico_set_program_name(${PROJECT_NAME} "Captar Som e LEDs")
pico_set_program_version(${PROJECT_NAME} "1.0")

# Gera o cabeçalho PIO a partir do ws2818b.pio fornecido
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/ws2818b.pio)

# Configura stdio
pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Bibliotecas Linkadas
target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_stdlib
        hardware_timer
        hardware_adc
        hardware_pio
        hardware_clocks
)

# Permite includes locais
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

# Gera o .uf2 e outros formatos
pico_add_extra_outputs(${PROJECT_NAME})