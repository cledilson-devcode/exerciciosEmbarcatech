add_executable(tarefa_u3c2_FreeRTOS_escalonamento
    main.c
    perifericos.c 
    tarefas.c 

)

# Corrige a saída para build/ em vez de build/src/
set_target_properties(tarefa_u3c2_FreeRTOS_escalonamento PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_sources(tarefa_u3c2_FreeRTOS_escalonamento PRIVATE
    ${FREERTOS_KERNEL_PATH}/portable/MemMang/heap_4.c
)

target_include_directories(tarefa_u3c2_FreeRTOS_escalonamento PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../inc

)

target_link_libraries(tarefa_u3c2_FreeRTOS_escalonamento
    pico_stdlib 
    FreeRTOS-Kernel
    hardware_adc
)

pico_add_extra_outputs(tarefa_u3c2_FreeRTOS_escalonamento)
