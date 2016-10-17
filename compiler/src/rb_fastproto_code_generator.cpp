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
            "namespace rb_fastproto_gen {"
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
            "#include <ruby/ruby.h>\n"
            "#include <limits>\n"
            "#include <type_traits>\n"
            "#include <new>\n"
            "#include <cstring>\n"
            "#include \"rb_fastproto_init.h\"\n"
            "#include \"$header_name$\"\n"
            "\n"
            "namespace rb_fastproto_gen {"
            "\n",
            "header_name", header_path_for_proto(file)
        );

        printer.Indent(); printer.Indent();

        printer.Print(
            "static VALUE package_rb_module = Qnil;\n"
        );
        // We need to store a VALUE for each class we are going to define
        for (int i = 0; i < file->message_type_count(); i++) {
            auto message_type = file->message_type(i);


            // Define a struct that ruby will wrap for this class.
            // note that we do this in here, not in the header file, because the names might not
            // be unique so we really don't want to export these symbols
            printer.Print(
                "struct $class_name$ {\n",
                "class_name", message_type->name()
            );
            printer.Indent(); printer.Indent();

            // Object constructor; called in ruby initialize method.
            printer.Print(
                "// Keep this so we know whether to delete a char array of memory, or delete the object\n"
                "// (thereby invoking its constructor)\n"
                "bool have_initialized;\n"
                "\n"
                "$class_name$() : have_initialized(true) { }\n"
                "\n",
                "class_name", message_type->name()
            );


            // a static method that defines this class in rubyland
            printer.Print(
                "static VALUE rb_cls;\n"
                "static void initialize_class() {\n"
            );
            printer.Indent(); printer.Indent();

            printer.Print(
                "rb_cls = rb_define_class_under(package_rb_module, \"$class_name$\", cls_fastproto_message);\n"
                "rb_define_alloc_func(rb_cls, &alloc);\n"
                "rb_define_method(rb_cls, \"initialize\", reinterpret_cast<VALUE(*)(...)>(&initialize), 0);\n"
                "\n",
                "class_name", message_type->name()
            );

            // The ruby class needs accessors defined
            for(int j =  0; j < message_type->field_count(); j++) {
                auto field = message_type->field(j);

                printer.Print(
                    "rb_define_method(rb_cls, \"$field_name$\", reinterpret_cast<VALUE(*)(...)>(&get_$field_name$), 0);\n"
                    "rb_define_method(rb_cls, \"$field_name$=\", reinterpret_cast<VALUE(*)(...)>(&set_$field_name$), 1);\n",
                    "field_name", field->name()
                );
            }

            printer.Outdent(); printer.Outdent();
            printer.Print("}\n\n");


            // a VALUE for each member.
            for (int j = 0; j < message_type->field_count(); j++) {
                auto field = message_type->field(j);

                printer.Print("VALUE _field_$field_name$;\n", "field_name", field->name());
                printer.Print("VALUE _set_field_$field_name$(VALUE val) {\n", "field_name", field->name());
                printer.Indent(); printer.Indent();

                // We need different validation depending on what type the field is
                switch (field->type()) {
                    case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                    case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
                        printer.Print(
                            "Check_Type(val, T_FIXNUM);\n"
                            "if (FIX2LONG(val) > std::numeric_limits<int32_t>::max()) rb_raise(rb_eTypeError, \"Not within limits\");\n"
                            "if (FIX2LONG(val) < std::numeric_limits<int32_t>::min()) rb_raise(rb_eTypeError, \"Not within limits\");\n"
                            "_field_$field_name$ = val;\n"
                            "return Qnil;\n",
                            "field_name", field->name()
                        );
                        break;
                    default:
                        break;
                        // Not implemented.
                }
                printer.Outdent(); printer.Outdent();
                printer.Print("}\n");

                printer.Print(
                    "VALUE _get_field_$field_name$() {\n"
                    "    return _field_$field_name$;\n"
                    "}\n",
                    "field_name", field->name()
                );

                // Define a static setter & getter that ruby can call, that casts our data struct and calls the instance method.
                printer.Print(
                    "static VALUE set_$field_name$(VALUE self, VALUE val) {\n"
                    "    $class_name$* cpp_self;\n"
                    "    Data_Get_Struct(self, $class_name$, cpp_self);\n"
                    "    return cpp_self->_set_field_$field_name$(val);\n"
                    "}\n"
                    "static VALUE get_$field_name$(VALUE self) {\n"
                    "    $class_name$* cpp_self;\n"
                    "    Data_Get_Struct(self, $class_name$, cpp_self);\n"
                    "    return cpp_self->_get_field_$field_name$();\n"
                    "}\n"
                    "\n",
                    "field_name", field->name(),
                    "class_name", message_type->name()
                );
            }

            // The message needs an alloc function, and a free function, and a mark function, for ruby.
            printer.Print(
                "static VALUE alloc(VALUE self) {\n"
                "    auto memory = new std::aligned_storage<sizeof($class_name$)>;\n"
                "    std::memset(memory, 0, sizeof($class_name$));\n"
                "    return Data_Wrap_Struct(self, &mark, &free, memory);\n"
                "}\n"
                "\n"
                "static VALUE initialize(VALUE self) {\n"
                "    // Use placement new to create the object\n"
                "    void* memory;\n"
                "    Data_Get_Struct(self, void*, memory);\n"
                "    new(memory) $class_name$();\n"
                "    return self;\n"
                "}\n"
                "\n"
                "static void free(char* memory) {\n"
                "    auto obj = reinterpret_cast<$class_name$*>(memory);\n"
                "    if (memory != nullptr && obj->have_initialized) {\n"
                "        delete obj;\n"
                "    } else {\n"
                "        delete memory;\n"
                "    }\n"
                "}\n"
                "\n"
                "static void mark(char* memory) {\n"
                "    auto obj = reinterpret_cast<$class_name$*>(memory);\n"
                "    obj->_mark();\n"
                "}\n"
                "\n",
                "class_name", message_type->name()
            );

            // The mark function needs to touch all of our value fields
            printer.Print("void _mark() {\n");
            printer.Indent(); printer.Indent();

            for (int j = 0; j < message_type->field_count(); j++) {
                auto field = message_type->field(j);

                printer.Print("rb_gc_mark(this->_field_$field_name$);\n", "field_name", field->name());
            }

            printer.Outdent(); printer.Outdent();
            printer.Print("};\n");


            printer.Outdent(); printer.Outdent();
            printer.Print("};\n");

            // For some silly reason we need to initialized its static members at translation-unit scope?
            printer.Print("VALUE $class_name$::rb_cls = Qnil;\n", "class_name", message_type->name());
        }
        printer.Print("\n");
        printer.Print(
            "extern \"C\" void _Init_$header_name$() {\n",
            "header_name", header_name_as_identifier(file)
        );
        printer.Indent(); printer.Indent();

        // We need to define a ruby module for the package to store message classes in.
        std::vector<std::string> package_elements;
        boost::algorithm::split(package_elements, file->package(), boost::is_any_of("."));
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

        // For every message in the file, we need to create a message class
        //     - Create a message class
        for (int i = 0; i < file->message_type_count(); i++) {
            auto message_type = file->message_type(i);

            printer.Print(
                "$class_name$::initialize_class();\n",
                "class_name", message_type->name()
            );
        }

        // Close off the _Init_header_name function
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");

        // Close off the rb_fastproto_gen namespace
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");
    }
}
