#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/compiler/plugin.pb.h>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include "rb_fastproto_code_generator.h"

// These are defined in resources
extern "C" {
    extern unsigned char runtime_extconf_rb[];
    extern unsigned int runtime_extconf_rb_len;
    extern unsigned char runtime_rb_fastproto_init_cpp[];
    extern unsigned int runtime_rb_fastproto_init_cpp_len;
    extern unsigned char runtime_rb_fastproto_init_h[];
    extern unsigned int runtime_rb_fastproto_init_h_len;
}

namespace rb_fastproto {
    void RBFastProtoCodeGenerator::write_entrypoint(
        const google::protobuf::FileDescriptor* file,
        google::protobuf::compiler::OutputDirectory *output_directory
    ) const {

        // Open the insertion point to include headers
        boost::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> header_insertion(
            output_directory->OpenForInsert("rb_fastproto_init.cpp", "init_file_header")
        );
        google::protobuf::io::Printer header_insertion_printer(header_insertion.get(), '$');

        boost::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> entrypoint_insertion(
            output_directory->OpenForInsert("rb_fastproto_init.cpp", "init_entrypoints")
        );
        google::protobuf::io::Printer entrypoint_insertion_printer(entrypoint_insertion.get(), '$');

        header_insertion_printer.Print(
            "#include \"$header$\"\n",
            "header", header_path_for_proto(file)
        );
        entrypoint_insertion_printer.Print(
            "rb_fastproto_gen::$package_ns$::_Init_$file_name$();\n",
            "package_ns", boost::join(rubyised_namespace_els(file), "::"),
            "file_name", header_name_as_identifier(file)
        );
    }

    // Static method that writes the extconf.rb and rb_fastproto_init files
    void add_entrypoint_files(google::protobuf::compiler::CodeGeneratorResponse &response) {

        auto fastproto_header_file = response.add_file();
        fastproto_header_file->set_name("rb_fastproto_init.h");
        fastproto_header_file->set_content(reinterpret_cast<char*>(runtime_rb_fastproto_init_h), runtime_rb_fastproto_init_h_len);

        auto fastproto_init_file = response.add_file();
        fastproto_init_file->set_name("rb_fastproto_init.cpp");
        fastproto_init_file->set_content(reinterpret_cast<char*>(runtime_rb_fastproto_init_cpp), runtime_rb_fastproto_init_cpp_len);

        // Add an extconf.rb so it can be compiled
        auto extconf_file = response.add_file();
        extconf_file->set_name("extconf.rb");
        extconf_file->set_content(reinterpret_cast<char*>(runtime_extconf_rb), runtime_extconf_rb_len);
    }
}