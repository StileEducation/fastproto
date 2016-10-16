#include <google/protobuf/compiler/plugin.h>
#include "rb_fastproto_code_generator.h"

int main(int argc, char** argv) {
    rb_fastproto::RBFastProtoCodeGenerator generator;
    return google::protobuf::compiler::PluginMain(argc, argv, &generator);
}
