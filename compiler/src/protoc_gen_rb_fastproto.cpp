#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/compiler/plugin.pb.h>
#include <google/protobuf/compiler/code_generator.h>
#include <iostream>
#include <unistd.h>
#include "rb_fastproto_code_generator.h"

namespace rb_fastproto {
    int PluginMain(int argc, char* argv[], const google::protobuf::compiler::CodeGenerator* generator);
}

int main(int argc, char** argv) {
    rb_fastproto::RBFastProtoCodeGenerator generator;
    return rb_fastproto::PluginMain(argc, argv, &generator);
}

// I copypasta'd this from plugin.cc in the protobuf compiler;
// This is how they recommend you inspect the request message before generation.
namespace rb_fastproto {
    int PluginMain(int argc, char* argv[], const google::protobuf::compiler::CodeGenerator* generator) {

        if (argc > 1) {
            std::cerr << argv[0] << ": Unknown option: " << argv[1] << std::endl;
            return 1;
        }

        google::protobuf::compiler::CodeGeneratorRequest request;
        if (!request.ParseFromFileDescriptor(STDIN_FILENO)) {
            std::cerr << argv[0] << ": protoc sent unparseable request to plugin."
                << std::endl;
            return 1;
        }

        std::string error_msg;
        google::protobuf::compiler::CodeGeneratorResponse response;

        // Before sending the response protobuf to our generator, add a file for our entrypoint
        // so that our generator can make use of the insertion points in it.
        add_entrypoint_files(response);

        if (google::protobuf::compiler::GenerateCode(request, *generator, &response, &error_msg)) {
            if (!response.SerializeToFileDescriptor(STDOUT_FILENO)) {
                std::cerr << argv[0] << ": Error writing to stdout." << std::endl;
                return 1;
            }
        } else {
            if (!error_msg.empty()) {
                std::cerr << argv[0] << ": " << error_msg << std::endl;
            }
            return 1;
        }
        return 0;
    }
}