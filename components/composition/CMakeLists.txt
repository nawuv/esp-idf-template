idf_component_register(SRCS "src/device.cpp" INCLUDE_DIRS "include"
    PRIV_REQUIRES application runtime sensors actuators core)
set_target_properties(${COMPONENT_LIB} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
