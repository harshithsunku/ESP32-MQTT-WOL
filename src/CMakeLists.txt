# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources}
                      REQUIRES esp_system esp_wifi esp_event nvs_flash esp_partition esp_timer spi_flash app_update bootloader_support lwip console)
