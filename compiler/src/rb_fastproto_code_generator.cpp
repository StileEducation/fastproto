#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "rb_fastproto_code_generator.h"

namespace rb_fastproto {

    RBFastProtoCodeGenerator::RBFastProtoCodeGenerator() { }

    bool RBFastProtoCodeGenerator::Generate(
        const google::protobuf::FileDescriptor *file,
        const std::string &parameter,
        google::protobuf::compiler::OutputDirectory *output_directory,
        std::string *error
    ) const {
        auto output_cpp_file_name = cpp_path_for_proto(file);
        boost::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> cpp_output(output_directory->Open(output_cpp_file_name));
        google::protobuf::io::Printer cpp_printer(cpp_output.get(), '$');

        auto output_header_file_name = header_path_for_proto(file);
        boost::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> header_output(output_directory->Open(output_header_file_name));
        google::protobuf::io::Printer header_printer(header_output.get(), '$');

        write_header(file, header_printer);
        write_cpp(file, cpp_printer);

        write_entrypoint(file, output_directory);

        return true;
    }

    void RBFastProtoCodeGenerator::write_header(
        const google::protobuf::FileDescriptor *file,
        google::protobuf::io::Printer &printer
    ) const {
        // Write an include guard in the header
        printer.Print(
            "#ifndef __$header_file_name$_H\n"
            "#define __$header_file_name$_H\n"
            "\n"
            "namespace pt_fastproto_gen {"
            "\n",
            "header_file_name", header_name_as_identifier(file)
        );
        printer.Indent(); printer.Indent();

        // Define a method that the overall-init will call to define the ruby classes & modules contained
        // in this file.
        printer.Print(
            "extern \"C\" void _Init_$header_file_name$();\n",
            "header_file_name", header_name_as_identifier(file)
        );

        printer.Outdent(); printer.Outdent();
        printer.Print(
            "}\n"
            "#endif\n"
        );
    }

    void RBFastProtoCodeGenerator::write_cpp(
        const google::protobuf::FileDescriptor *file,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print(
            "#include \"$header_name$\"\n"
            "\n"
            "namespace pt_fastproto_gen {"
            "\n",
            "header_name", header_path_for_proto(file)
        );

        printer.Indent(); printer.Indent();

        printer.Print(
            "extern \"C\" void _Init_$header_name$() {\n",
            "header_name", header_name_as_identifier(file)
        );
        printer.Indent(); printer.Indent();

        // Close off the _Init_header_name function
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");

        // Close off the pt_fastproto_gen namespace
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");
    }
}
