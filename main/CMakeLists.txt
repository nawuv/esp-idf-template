idf_component_register(SRCS "app_main.cpp" "device.cpp" INCLUDE_DIRS "."
    PRIV_REQUIRES configuration application platform drivers networking log)
set_target_properties(${COMPONENT_LIB} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>)
