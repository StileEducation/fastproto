CMAKE_MINIMUM_REQUIRED (VERSION 3.0)
PROJECT (rb_fastproto_compiler)
ADD_EXECUTABLE (rb_fastproto_compiler
    src/protoc_gen_rb_fastproto.cpp
    src/rb_fastproto_code_generator.cpp
    src/rb_fastproto_code_generator_utils.cpp
)
TARGET_LINK_LIBRARIES (rb_fastproto_compiler
    protoc protobuf boost_filesystem boost_system
)
set(CMAKE_CXX_FLAGS " --std=c++14")