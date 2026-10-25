idf_component_register(SRCS "src/network_config.cpp" "src/nvs_config.cpp" INCLUDE_DIRS "include" PRIV_REQUIRES nvs_flash)
set_target_properties(${COMPONENT_LIB} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>)
