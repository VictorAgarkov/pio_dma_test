cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH "~/src/pico/pico-sdk")
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

set(PROJECT_NAME "pio_dma_test")

set(CHERRYUSB_DIR "./CherryUSB")


#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

project(${PROJECT_NAME} C CXX ASM)

pico_sdk_init()

add_executable(${PROJECT_NAME}
	src/main.c
	src/audio_v2_mic_speaker_multichan_template.c

	${CHERRYUSB_DIR}/core/usbd_core.c                 
	${CHERRYUSB_DIR}/class/audio/usbd_audio.c         
	${CHERRYUSB_DIR}/port/rp2040/usb_dc_rp2040.c    
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${CHERRYUSB_DIR}/core
	${CHERRYUSB_DIR}/common
	${CHERRYUSB_DIR}/class/audio	
	${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
	OUTPUT_NAME "../bin/${PROJECT_NAME}"
)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/dma_test.pio)

# Линковка с Pico SDK
target_link_libraries(${PROJECT_NAME} 
	pico_stdlib 
	cmsis_core 
	pico_multicore
	hardware_pio	
	hardware_dma
)

pico_add_extra_outputs(${PROJECT_NAME})

# выводим сколько ресурсов МК использовано
target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--print-memory-usage")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND arm-none-eabi-size $<TARGET_FILE:${PROJECT_NAME}>
	COMMENT "MCU memory usage:"
)

