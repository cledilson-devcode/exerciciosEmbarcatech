add_executable(Tarefa_FreeRTOS_A
    main.c
    botoes.c 
    leds_rgb.c 
    tarefas_rgb_smp.c
    task_gerenciadora.c 

)

# Corrige a saída para build/ em vez de build/src/
set_target_properties(Tarefa_FreeRTOS_A PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_sources(Tarefa_FreeRTOS_A PRIVATE
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
)

target_include_directories(Tarefa_FreeRTOS_A PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../inc

)

target_link_libraries(Tarefa_FreeRTOS_A
    pico_stdlib 
    FreeRTOS-Kernel
)

pico_add_extra_outputs(Tarefa_FreeRTOS_A)
