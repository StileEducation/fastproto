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
        auto fastproto_init_file = response.add_file();
        fastproto_init_file->set_name("rb_fastproto_init.cpp");
        fastproto_init_file->set_content(
            "// Generated code that calls all the entrypoints\n"
            "\n"
            "// @@protoc_insertion_point(init_file_header)\n"
            "\n"
            "extern \"C\" void Init_fastproto_gen(void) {\n"
            "    // @@protoc_insertion_point(init_entrypoints)\n"
            "}\n"
        );

        // Add an extconf.rb so it can be compiled
        auto extconf_file = response.add_file();
        extconf_file->set_name("extconf.rb");
        extconf_file->set_content(
            "require 'mkmf'\n"
            "dir_config('protobuf')\n"
            "dir_config('boost')\n"
            "$CXXFLAGS << ' --std=c++14'\n"
            "$LOCAL_LIBS << ' -lprotobuf'\n"
            // All of our objects are in subdirectories (that match the original proto subdirectories)
            // but mkmf only compiles stuff in the toplevel. Add targets to $objs representing *every*
            // cpp/cc file in the tree.
            "$objs =  Dir['**/*.{cpp,cc,c}'].map { |f|\n"
            "    File.join(File.dirname(f), File.basename(f, '.*')) + '.' + CONFIG['OBJEXT']\n"
            "}\n"
            "create_makefile('fastproto_gen')\n"
            // We now need to hack at the generated makefile to add a cleanup job (using a bash extglob)
            // to clean up the extra targets we added.
            "makefile_text = File.read('Makefile')\n"
            "makefile_text.gsub!(/^\\s*SHELL\\s*=.*$/, 'SHELL=/bin/bash -O extglob -c')\n"
            "makefile_text += <<-EOS\n"
            "    CLEANOBJS:= $(CLEANOBJS) **/*.#{CONFIG['OBJEXT']}\n"
            "EOS\n"
            "File.open('Makefile', 'w') { |f| f.write makefile_text }\n"
        );

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