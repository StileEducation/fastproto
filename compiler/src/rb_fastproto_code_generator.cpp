#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "rb_fastproto_code_generator.h"

namespace rb_fastproto {
    bool RBFastProtoCodeGenerator::Generate(
        const google::protobuf::FileDescriptor *file,
        const std::string &parameter,
        google::protobuf::compiler::OutputDirectory *output_directory,
        std::string *error
    ) const {
        // Output the .rb_fastproto.cpp file to the same logical location
        // as the input file name.
        const std::string file_name = file->name();
        std::string output_cpp_file_name = file->name();
        std::string output_header_file_name = file->name();
        std::size_t loc = file_name.rfind(".");
        // If there is no . in the filename, not much we can do.
        if (loc == std::string::npos) {
            *error = "Filename contains no period";
            return false;
        }

        output_cpp_file_name.erase(loc, file_name.length() - loc);
        output_cpp_file_name.append(".rb_fastproto.cpp");
        output_header_file_name.erase(loc, file_name.length() - loc);
        output_header_file_name.append(".rb_fastproto.h");

        boost::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> cpp_output(output_directory->Open(output_cpp_file_name));
        google::protobuf::io::Printer cpp_printer(cpp_output.get(), '$');
        boost::scoped_ptr<google::protobuf::io::ZeroCopyOutputStream> header_output(output_directory->Open(output_header_file_name));
        google::protobuf::io::Printer header_printer(header_output.get(), '$');

        write_header(file, output_header_file_name, header_printer);
        write_cpp(file, output_cpp_file_name, output_header_file_name, cpp_printer);

        return true;
    }

    void RBFastProtoCodeGenerator::write_header(
        const google::protobuf::FileDescriptor *file,
        const std::string &full_header_file_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Write an include guard in the header
        printer.Print(
            "#ifndef __$header_file_name$_H\n"
            "#define __$header_file_name$_H\n"
            "\n"
            "namespace pt_fastproto_gen {"
            "\n",
            "header_file_name", header_name_as_identifier(full_header_file_name)
        );
        printer.Indent(); printer.Indent();

        // Define a method that the overall-init will call to define the ruby classes & modules contained
        // in this file.
        printer.Print(
            "extern \"C\" _Init_$header_file_name$();\n",
            "header_file_name", header_name_as_identifier(full_header_file_name)
        );

        printer.Outdent(); printer.Outdent();
        printer.Print(
            "}\n"
            "#endif\n"
        );
    }

    void RBFastProtoCodeGenerator::write_cpp(
        const google::protobuf::FileDescriptor *file,
        const std::string &full_cpp_file_name,
        const std::string &full_header_file_name,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print(
            "#include \"$header_name$\"\n"
            "\n"
            "naespace pt_fastproto_gen {"
            "\n",
            "header_name", full_header_file_name
        );

        printer.Indent(); printer.Indent();

        printer.Print(
            "extern \"C\" _Init_$header_name$() {\n",
            "header_name", header_name_as_identifier(full_header_file_name)
        );
        printer.Indent(); printer.Indent();

        // Close off the _Init_header_name function
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");

        // Close off the pt_fastproto_gen namespace
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");
    }

    std::string RBFastProtoCodeGenerator::header_name_as_identifier(const std::string &header_file_name) const {
        boost::filesystem::path header_path(header_file_name);
        boost::filesystem::path header_path_no_ext(header_path.parent_path());
        header_path_no_ext = header_path_no_ext / header_path.stem();

        auto header_file_name_with_underscores = boost::replace_all_copy(
            header_path_no_ext.string(),
            std::string(1, boost::filesystem::path::preferred_separator),
            "_"
        );
        boost::replace_all(header_file_name_with_underscores, ".", "_");
        boost::to_upper(header_file_name_with_underscores);
        return header_file_name_with_underscores;
    }
}