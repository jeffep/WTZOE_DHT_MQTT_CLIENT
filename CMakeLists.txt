
# Build the executable based on the source files
set(TARGET_NAME WZTOE_DHT_MQTT_CLIENT)

add_executable(${TARGET_NAME}

  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/WZTOE/WZTOE_DHT_MQTT_CLIENT/main.c
  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/WZTOE/WZTOE_DHT_MQTT_CLIENT/dualtimer.c
  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/WZTOE/WZTOE_DHT_MQTT_CLIENT/W7500x_it.c
  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/WZTOE/WZTOE_DHT_MQTT_CLIENT/retarget.c
  ${CMAKE_SOURCE_DIR}/Libraries/CMSIS/Device/WIZnet/W7500/Source/GCC/startup_w7500.s
  )

target_include_directories(${TARGET_NAME} PUBLIC
  ${CMAKE_SOURCE_DIR}/Projects/W7500x_StdPeriph_Examples/WZTOE/WZTOE_DHT_MQTT_CLIENT
  C:\\Users\\Jeffrey\\.pico-sdk\\sdk\\1.5.1\\src\\common\\pico_stdlib\\include
  C:\\Users\\Jeffrey\\.pico-sdk\\sdk\\1.5.1\\src\\common\\pico_base\\include
  C:\\Users\\Jeffrey\\.pico-sdk\\sdk\\1.5.1\\lib\\mbedtls\\include\\mbedtls
)
target_link_libraries(${TARGET_NAME} PRIVATE
    SYSTEM_W7500X_FILES  
    GPIO_FILES
    UART_FILES
    ADC_FILES
    CRG_FILES
    DUALTIMER_FILES
    EXTI_FILES
    FLASH_FILES
    MIIM_FILES
    MISC_FILES
    PWM_FILES
    RNG_FILES
    RTC_FILES
    SSP_FILES
    WDT_FILES
    WZTOE_FILES  
    SOCKET_FILES 
    LOOPBACK_FILES
    WIZCHIP_CONF_FILES
    MQTT_FILES
)

target_compile_definitions(WZTOE_DHT_MQTT_CLIENT PUBLIC DEBUG_PRINT=printf)

complie_link_option(${TARGET_NAME})
create_binary_file(${TARGET_NAME})
create_elf_file(${TARGET_NAME})

