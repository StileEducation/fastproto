CMAKE_MINIMUM_REQUIRED (VERSION 3.0)
PROJECT (rb_fastproto_compiler)

# Compile runtime files using xxd -i, taken from
# http://stackoverflow.com/questions/14422507/cmake-run-xxd-as-custom-command
SET(RUNTIME_RESOURCES
    runtime/extconf.rb
    runtime/rb_fastproto_init.cpp
    runtime/rb_fastproto_init.h
)

FOREACH(INPUT_FILE ${RUNTIME_RESOURCES})
    SET(OUTPUT_FILE ${INPUT_FILE}.res.c)
    ADD_CUSTOM_COMMAND(
        OUTPUT ${OUTPUT_FILE}
        COMMAND xxd -i `realpath --relative-to=. "${INPUT_FILE}" ` "${OUTPUT_FILE}"
        COMMENT "Compiling ${INPUT_FILE} to binary"
    )
    LIST(APPEND COMPILED_RUNTIME_RESOURCES ${OUTPUT_FILE})
ENDFOREACH()

ADD_EXECUTABLE (rb_fastproto_compiler
    ${COMPILED_RUNTIME_RESOURCES}
    src/protoc_gen_rb_fastproto.cpp
    src/rb_fastproto_code_generator.cpp
    src/rb_fastproto_code_generator_utils.cpp
    src/rb_fastproto_code_generator_entrypoint.cpp
)
TARGET_LINK_LIBRARIES (rb_fastproto_compiler
    protoc protobuf boost_filesystem boost_system
)
set(CMAKE_CXX_FLAGS " --std=c++14")