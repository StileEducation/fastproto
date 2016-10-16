#include <google/protobuf/io/zero_copy_stream.h>
#include <boost/scoped_ptr.hpp>
#include <iostream>
#include "rb_fastproto_code_generator.h"

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
            "_Init_$init_method$();\n",
            "init_method", header_name_as_identifier(file)
        );
    }
}