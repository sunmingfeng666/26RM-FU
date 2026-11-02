# Standalone lamp/key test target; the CAN game still has five participants.
if(NOT DEFINED ENERGY_EXECUTABLE_NAME)
    message(FATAL_ERROR "Open the repository root and select slave7-Debug/Release.")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_PROJECT_NAME ${ENERGY_EXECUTABLE_NAME})
set(F1_DRIVERS "${CMAKE_CURRENT_SOURCE_DIR}/../fu_slave1/Drivers")
add_executable(${CMAKE_PROJECT_NAME}
    Core/Src/main.c Core/Src/stm32f1xx_hal_msp.c Core/Src/stm32f1xx_it.c
    Core/Src/gpio.c Core/Src/dma.c Core/Src/tim.c
    Core/Src/system_stm32f1xx.c Core/Src/syscalls.c Core/Src/sysmem.c
    startup_stm32f103xb.s
    user/APP/slave_key_test.c user/BSP/ws2812.c)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc user/APP user/BSP
    ${F1_DRIVERS}/STM32F1xx_HAL_Driver/Inc ${F1_DRIVERS}/STM32F1xx_HAL_Driver/Inc/Legacy
    ${F1_DRIVERS}/CMSIS/Device/ST/STM32F1xx/Include ${F1_DRIVERS}/CMSIS/Include)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE USE_HAL_DRIVER STM32F103xB $<$<CONFIG:Debug>:DEBUG>)
foreach(driver hal hal_cortex hal_rcc hal_rcc_ex hal_gpio hal_gpio_ex hal_dma hal_flash hal_flash_ex hal_pwr hal_exti hal_tim hal_tim_ex)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${F1_DRIVERS}/STM32F1xx_HAL_Driver/Src/stm32f1xx_${driver}.c)
endforeach()
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE m)
include(${CMAKE_SOURCE_DIR}/cmake/slave_output.cmake)
