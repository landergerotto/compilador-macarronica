set(MACARONICA_GENERATED_C "${CMAKE_CURRENT_LIST_DIR}/../../build/esp32-generated/prog_gerado.c" CACHE FILEPATH "C99 produzido pelo compilador Macaronica")
if(NOT EXISTS "${MACARONICA_GENERATED_C}")
    message(FATAL_ERROR "Gere primeiro o programa Macaronica: arquivo C ausente: ${MACARONICA_GENERATED_C}")
endif()
idf_component_register(
    SRCS "${MACARONICA_GENERATED_C}" "../../runtime/zl_runtime_esp32.c"
         "../../runtime/zl_plataforma_idf.c"
    INCLUDE_DIRS "../../runtime"
    REQUIRES esp_driver_gpio esp_driver_uart freertos
)
set_property(TARGET ${COMPONENT_LIB} PROPERTY C_STANDARD 99)
set_property(TARGET ${COMPONENT_LIB} PROPERTY C_STANDARD_REQUIRED ON)