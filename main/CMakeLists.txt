idf_component_register(SRCS "app_main.cpp" PRIV_REQUIRES composition)
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>)
