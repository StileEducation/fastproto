#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/filesystem.hpp>
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
        printer.Print(
            "#include <ruby/ruby.h>\n"
            "#include <vector>\n"
            "#include <utility>\n"
            "#include \"$pb_header_name$\"\n"
            "\n",
            "pb_header_name", cpp_proto_header_path_for_proto(file)
        );

        // Write an include guard in the header
        printer.Print(
            "#ifndef __$header_file_name$_H\n"
            "#define __$header_file_name$_H\n"
            "\n"
            "namespace rb_fastproto_gen {\n",
            "header_file_name", header_name_as_identifier(file)
        );
        printer.Indent();

        // Add namespaces for the generated proto package
        auto namespace_parts = rubyised_namespace_els(file);
        for (auto&& ns : namespace_parts) {
            printer.Print("namespace $ns$ {\n", "ns", ns);
            printer.Indent();
        }

        // We need to export our classes in the header file, so that other messages can
        // serialize this one as a part of their operation.
        for (int i = 0; i < file->message_type_count(); i++) {
            auto message_type = file->message_type(i);
            write_header_message_struct_definition(file, message_type, printer);
        }

        // Let's put services in this header as well. Because why not. Computers.
        for (int i = 0; i < file->service_count(); i++) {
            auto service = file->service(i);
            write_header_service_struct_definition(file, service, printer);

            for (int j = 0; j < service->method_count(); j++) {
                auto method = service->method(j);
                write_header_method_struct_definition(file, method, printer);
            }
        }

        // Define a method that the overall-init will call to define the ruby classes & modules contained
        // in this file.
        printer.Print(
            "void _Init();\n"
        );

        for (auto&& ns : namespace_parts) {
            printer.Outdent();
            printer.Print("}\n");
        }

        printer.Outdent();
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
            "#include <ruby/ruby.h>\n"
            "#include <ruby/encoding.h>\n"
            "#include <ruby/thread.h>\n"
            "#include <limits>\n"
            "#include <type_traits>\n"
            "#include <new>\n"
            "#include <cstring>\n"
            "#include <functional>\n"
            "#include <tuple>\n"
            "#include <typeinfo>\n"
            "#include \"rb_fastproto_init.h\"\n"
            "#include \"$header_name$\"\n"
            "#include \"$pb_header_name$\"\n"
            "\n"
            "namespace rb_fastproto_gen {\n",
            "header_name", header_path_for_proto(file),
            "pb_header_name", cpp_proto_header_path_for_proto(file)
        );

        printer.Indent();

        // Add namespaces for the generated proto package
        auto namespace_parts = rubyised_namespace_els(file);
        for (auto&& ns : namespace_parts) {
            printer.Print("namespace $ns$ {\n", "ns", ns);
            printer.Indent();
        }

        printer.Print("static VALUE package_rb_module = Qnil;\n");

        // Collect the class names we generate.
        std::vector<std::string> class_names;

        for (int i = 0; i < file->message_type_count(); i++) {
            auto message_type = file->message_type(i);
            class_names.push_back(write_cpp_message_struct(file, message_type, printer));
        }

        for (int i = 0; i < file->service_count(); i++) {
            auto service = file->service(i);
            class_names.push_back(write_cpp_service_struct(file, service, printer));

            for (int j = 0; j < service->method_count(); j++) {
                auto method = service->method(j);
                class_names.push_back(write_cpp_method_struct(file, method, printer));
            }
        }

        // Write an Init function that gets called from the ruby gem init.
        write_cpp_module_init(file, class_names, printer);

        for (auto&& ns : namespace_parts) {
            printer.Outdent();
            printer.Print("}\n");
        }

        printer.Outdent();

        printer.Print("}\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_module_init(
        const google::protobuf::FileDescriptor* file,
        const std::vector<std::string> &class_names,
        google::protobuf::io::Printer &printer
    ) const {
        printer.Print("void _Init() {\n");

        printer.Indent();

        // We need to define a ruby module for the package to store message classes in.
        auto package_elements = rubyised_namespace_els(file);

        // The first scope is special because we need to use rb_define_module, not rb_define_module_under
        printer.Print(
            "package_rb_module = rb_define_module(\"$package_el$\");\n",
            "package_el", package_elements[0]
        );

        for (int i = 1; i < package_elements.size(); i++) {
            printer.Print(
                "package_rb_module = rb_define_module_under(package_rb_module, \"$package_el$\");\n",
                "package_el", package_elements[i]
            );
        }

        for (auto&& class_name : class_names) {
            printer.Print("$class_name$::initialize_class();\n", "class_name", class_name);
        }

        printer.Outdent();

        printer.Print("}\n");
    }
}
