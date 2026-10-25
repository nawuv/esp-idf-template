idf_component_register(SRCS "app_main.cpp" PRIV_REQUIRES application runtime sensors actuators core freertos log)
set_target_properties(${COMPONENT_LIB} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
