idf_component_register(SRCS "src/runtime.cpp" "src/i2c.cpp" INCLUDE_DIRS "include" REQUIRES application freertos PRIV_REQUIRES esp_timer log esp_driver_i2c esp_driver_gpio)
set_target_properties(${COMPONENT_LIB} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>)
