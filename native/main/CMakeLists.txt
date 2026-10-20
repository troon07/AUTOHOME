set(COMMON "${CMAKE_CURRENT_LIST_DIR}/../../firmware/common")
idf_component_register(
    SRCS "app_entry.cpp" "native_hal.cpp"
         "${COMMON}/protocol/clawgrid_protocol.c"
         "${COMMON}/safety_kernel/safety_kernel.c"
         "${COMMON}/safety_kernel/safety_selftest.c"
         "${COMMON}/neural_nlp/neural_agent.c"
         "${COMMON}/runtime/memory_runtime.cpp"
         "${COMMON}/runtime/runtime_selftest.cpp"
    INCLUDE_DIRS "." "${COMMON}/protocol" "${COMMON}/safety_kernel"
         "${COMMON}/neural_nlp" "${COMMON}/runtime" "${COMMON}/config"
    REQUIRES driver esp_wifi esp_event esp_netif esp_timer nvs_flash
             esp_http_server app_update mbedtls spi_flash lwip)
