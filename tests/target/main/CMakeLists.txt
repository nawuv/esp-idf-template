idf_component_register(SRCS "test_main.cpp" PRIV_REQUIRES unity)
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>)
