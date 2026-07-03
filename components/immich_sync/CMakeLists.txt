idf_component_register(SRCS "immich_sync.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_client json mbedtls secrets)
