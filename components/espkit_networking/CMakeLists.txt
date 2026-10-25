idf_component_register(SRCS "src/telemetry.cpp" "src/esp_network.cpp" INCLUDE_DIRS "include"
    REQUIRES core espkit_configuration mqtt esp_event PRIV_REQUIRES esp_netif esp_wifi mbedtls esp_hw_support lwip)
set_target_properties(${COMPONENT_LIB} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>)
