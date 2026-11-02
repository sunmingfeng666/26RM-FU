# Dedicated motor firmware. Use the repository root and motor-Debug/Release presets.
if(NOT DEFINED ENERGY_EXECUTABLE_NAME)
    message(FATAL_ERROR "Open the repository root CMakeLists.txt and select motor-Debug.")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_PROJECT_NAME ${ENERGY_EXECUTABLE_NAME})
set(F1_DRIVERS "${CMAKE_CURRENT_SOURCE_DIR}/../fu_slave1/Drivers")
add_executable(${CMAKE_PROJECT_NAME}
    Core/Src/main.c Core/Src/stm32f1xx_hal_msp.c Core/Src/stm32f1xx_it.c
    Core/Src/system_stm32f1xx.c Core/Src/syscalls.c Core/Src/sysmem.c
    startup_stm32f103xb.s
    user/APP/motor_control.c user/APP/motor_board.c user/APP/motor_remote.c
    user/BSP/motor_can.c user/BSP/motor_uart.c user/BSP/motor_uart_hw.c ../../common/motor_link.c)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc user/APP user/BSP ../../common
    ${F1_DRIVERS}/STM32F1xx_HAL_Driver/Inc ${F1_DRIVERS}/STM32F1xx_HAL_Driver/Inc/Legacy
    ${F1_DRIVERS}/CMSIS/Device/ST/STM32F1xx/Include ${F1_DRIVERS}/CMSIS/Include)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE USE_HAL_DRIVER STM32F103xB $<$<CONFIG:Debug>:DEBUG>)
# Reuse the existing ST HAL/CMSIS distribution, not any slave application source.
foreach(driver hal hal_can hal_cortex hal_rcc hal_rcc_ex hal_gpio hal_gpio_ex hal_flash hal_flash_ex hal_pwr hal_exti hal_uart hal_dma hal_adc hal_adc_ex)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${F1_DRIVERS}/STM32F1xx_HAL_Driver/Src/stm32f1xx_${driver}.c)
endforeach()
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE m)
include(${CMAKE_SOURCE_DIR}/cmake/slave_output.cmake)
