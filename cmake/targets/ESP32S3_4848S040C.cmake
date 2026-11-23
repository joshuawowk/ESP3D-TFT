if(ESP32S3_4848S040C)
    set(TFT_TARGET "ESP32S3_4848S040C")
    set(SDKCONFIG ${CMAKE_SOURCE_DIR}/hardware/common/sdkconfig/ESP32-S3/with_PSRAM/16MB_Flash)
    list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/hardware/ESP32S3_4848S040C/components)
    list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/hardware/drivers_video_rgb)
    add_compile_options("-I${CMAKE_SOURCE_DIR}/hardware/ESP32S3_4848S040C/components/bsp")
    # No USB-OTG: GPIO19/20 (USB D-/D+) are used by touch I2C and RGB data
    set (RESOLUTION_SCREEN "res_480_480")
endif()
