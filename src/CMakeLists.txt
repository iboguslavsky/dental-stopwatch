FILE(GLOB_RECURSE app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

idf_component_register(
    SRCS ${app_sources}
    INCLUDE_DIRS "." "${CMAKE_SOURCE_DIR}/include"
    REQUIRES esp_wifi nvs_flash esp_netif esp_event driver esp_timer esp_hw_support esp_http_client json
)
