add_executable(Tarefa_FreeRTOS_C
    main.c
    semaforo_rgb.c
    ssd1306_init.c
    ssd1306_text.c
    ssd1306_utils.c
    oled_setup.c
    digitos_grandes_utils.c 
    numeros_grandes.c
    fsm_semaforo.c
    display_contador.c 
)

# Corrige a saída para build/ em vez de build/src/
set_target_properties(Tarefa_FreeRTOS_C PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_sources(Tarefa_FreeRTOS_C PRIVATE
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
)

target_include_directories(Tarefa_FreeRTOS_C PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../inc

)

target_link_libraries(Tarefa_FreeRTOS_C 
    pico_stdlib 
    FreeRTOS-Kernel
    hardware_adc
    hardware_i2c
    hardware_gpio
    hardware_pwm
)

pico_add_extra_outputs(Tarefa_FreeRTOS_C)
