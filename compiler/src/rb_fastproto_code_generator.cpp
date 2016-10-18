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
            "namespace rb_fastproto_gen {"
            "\n",
            "header_name", header_path_for_proto(file),
            "pb_header_name", cpp_proto_header_path_for_proto(file)
        );

        printer.Indent(); printer.Indent();

        printer.Print(
            "static VALUE package_rb_module = Qnil;\n"
        );
        // Define a struct for each message we have
        // Collect the class names that got written.
        std::vector<std::string> class_names;
        for (int i = 0; i < file->message_type_count(); i++) {
            auto message_type = file->message_type(i);
            class_names.push_back(write_cpp_message_struct(file, message_type, printer));
        }

        // Write an Init function that gets called from the ruby gem init.
        write_cpp_message_module_init(file, class_names, printer);

        // Close off the rb_fastproto_gen namespace
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");
    }

    std::string RBFastProtoCodeGenerator::write_cpp_message_struct(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        google::protobuf::io::Printer &printer
    ) const {
        // This bit is responsible for writing out the message struct.

        // Prefix the class name so we don't collide with the C++ generatec code.
        std::string class_name(message_type->name());
        class_name.insert(0, "RB");

        // Define a struct that ruby will wrap for this class.
        // note that we do this in here, not in the header file, because the names might not
        // be unique so we really don't want to export these symbols
        printer.Print(
            "struct $class_name$ {\n",
            "class_name", class_name
        );
        printer.Indent(); printer.Indent();

        // The instance constructor for a protobuf message
        write_cpp_message_struct_constructor(file, message_type, class_name, printer);
        // a static method that defines this class in rubyland
        write_cpp_message_struct_static_initializer(file, message_type, class_name, printer);
        // Static accessors for each field, so ruby can call them
        write_cpp_message_struct_accessors(file, message_type, class_name, printer);
        // The message needs an alloc function, and a free function, and a mark function, for ruby.
        // It also needs a static initialize method to use as a factory.
        write_cpp_message_struct_allocators(file, message_type, class_name, printer);
        // Define validation methods
        write_cpp_message_struct_validator(file, message_type, class_name, printer);
        // Define serialization methods
        write_cpp_message_struct_serializer(file, message_type, class_name, printer);
        // Desserialization methods
        write_cpp_message_struct_parser(file, message_type, class_name, printer);

        printer.Outdent(); printer.Outdent();
        printer.Print("};\n");

        // For some silly reason we need to initialized its static members at translation-unit scope?
        printer.Print("VALUE $class_name$::rb_cls = Qnil;\n", "class_name", class_name);

        return class_name;
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_constructor(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Object constructor; called in ruby initialize method.
        printer.Print(
            "// Keep this so we know whether to delete a char array of memory, or delete the object\n"
            "// (thereby invoking its constructor)\n"
            "bool have_initialized;\n"
            "// Now create a C++ message, which backs this one.\n"
            "$cpp_proto_class$ cpp_proto;\n"
            "// This holds the last validation exception we caught when assigning a field from\n"
            "// ruby to C. It will get raised when validate! is called.\n"
            "VALUE last_validation_exception;\n"
            "\n"
            "$class_name$() : \n"
            "    have_initialized(true),\n"
            "    cpp_proto(),\n"
            "    last_validation_exception(Qnil) {\n",

            "class_name", class_name,
            "cpp_proto_class", cpp_proto_class_name(message_type)
        );
        printer.Indent(); printer.Indent();

        // TODO: Set fields from the constructor.

        printer.Outdent(); printer.Outdent();
        printer.Print("}\n\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_static_initializer(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Defines the static initialize_class method for this struct

        printer.Print(
            "static VALUE rb_cls;\n"
            "static void initialize_class() {\n"
        );
        printer.Indent(); printer.Indent();

        printer.Print(
            "rb_cls = rb_define_class_under(package_rb_module, \"$ruby_class_name$\", cls_fastproto_message);\n"
            "rb_define_alloc_func(rb_cls, &alloc);\n"
            "rb_define_method(rb_cls, \"initialize\", reinterpret_cast<VALUE(*)(...)>(&initialize), 0);\n"
            "rb_define_method(rb_cls, \"validate!\", reinterpret_cast<VALUE(*)(...)>(&validate), 0);\n"
            "rb_define_method(rb_cls, \"serialize_to_string\", reinterpret_cast<VALUE(*)(...)>(&serialize_to_string), 0);\n"
            "rb_define_method(rb_cls, \"parse\", reinterpret_cast<VALUE(*)(...)>(&parse), 1);\n"
            "\n",
            "ruby_class_name", message_type->name(),
            "class_name", class_name
        );

        // The ruby class needs accessors defined
        for(int i =  0; i < message_type->field_count(); i++) {
            auto field = message_type->field(i);

            printer.Print(
                "rb_define_method(rb_cls, \"$field_name$\", reinterpret_cast<VALUE(*)(...)>(&get_$field_name$), 0);\n"
                "rb_define_method(rb_cls, \"$field_name$=\", reinterpret_cast<VALUE(*)(...)>(&set_$field_name$), 1);\n",
                "field_name", field->name()
            );
        }

        printer.Outdent(); printer.Outdent();
        printer.Print("}\n\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_accessors(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Static methods that ruby calls as accessors
        for (int j = 0; j < message_type->field_count(); j++) {
            auto field = message_type->field(j);

            // We are doing ruby type conversions.
            printer.Print(
                "static VALUE set_$field_name$(VALUE self, VALUE val) {\n"
                "    $class_name$* cpp_self;\n"
                "    Data_Get_Struct(self, $class_name$, cpp_self);\n"
                "\n",
                "field_name", field->name(),
                "class_name", class_name
            );
            printer.Indent(); printer.Indent();

            // Depending on the ruby class, we do a different conversion
            // The numeric _S macros are defined in generator_entrypoint.cpp
            // and wrap the ruby macros to disable float-to-int coersion
            std::string cpp_value_type;
            std::string conversion_macro;
            switch (field->type()) {
                case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
                    cpp_value_type = "int32_t";
                    conversion_macro = "NUM2INT_S($field_local$)";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
                case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
                    cpp_value_type = "uint32_t";
                    conversion_macro = "NUM2UINT_S($field_local$)";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
                case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
                    cpp_value_type = "int64_t";
                    conversion_macro = "NUM2LONG_S($field_local$)";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
                case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
                    cpp_value_type = "uint64_t";
                    conversion_macro = "NUM2ULONG_S($field_local$)";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
                    cpp_value_type = "float";
                    conversion_macro = "static_cast<float>(NUM2DBL($field_local$))";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
                    cpp_value_type = "double";
                    conversion_macro = "NUM2DBL($field_local$)";
                    break;
                default:
                    break;
            }

            // The gymnastics with the the dangerous_func lambda is so we can catch any exception
            // that gets raised in the conversion macro and re-raise it when validate! is called.
            printer.Print(
                ("struct dangerous_func_data {\n"
                "    $cpp_value_type$ converted_value;\n"
                "    VALUE value_to_convert;\n"
                "};\n"
                "auto dangerous_func = [](VALUE data_as_value) -> VALUE {\n"
                "    // Unwrap our 'VALUE' into an actual pointer\n"
                "    auto data = reinterpret_cast<dangerous_func_data*>(NUM2ULONG(data_as_value));\n"
                "    data->converted_value = " + conversion_macro + ";\n"
                "    return Qnil;\n"
                "};\n"
                "int exc_status;\n"
                "dangerous_func_data data_struct;\n"
                "data_struct.value_to_convert = val;\n"
                "rb_protect(dangerous_func, ULONG2NUM(reinterpret_cast<uint64_t>(&data_struct)), &exc_status);\n"
                "if (exc_status) {\n"
                "    // Exception!\n"
                "    cpp_self->last_validation_exception = rb_errinfo();\n"
                "    rb_set_errinfo(Qnil);\n"
                "} else {\n"
                "     cpp_self->cpp_proto.set_$field_name$(data_struct.converted_value);\n"
                "}").c_str(),
                "field_name", field->name(),
                "field_local", "data->value_to_convert",
                "cpp_value_type", cpp_value_type
            );

            printer.Print("return Qnil;\n");
            printer.Outdent(); printer.Outdent();
            printer.Print("}\n");

            printer.Print(
                "static VALUE get_$field_name$(VALUE self, VALUE val) {\n"
                "    $class_name$* cpp_self;\n"
                "    Data_Get_Struct(self, $class_name$, cpp_self);\n"
                "\n",
                "field_name", field->name(),
                "class_name", class_name
            );
            printer.Indent(); printer.Indent();

            // We also need a getter.
            // Tis is slightly safer and needs no rescue.
            switch (field->type()) {
                case google::protobuf::FieldDescriptor::Type::TYPE_INT32:
                case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED32:
                    conversion_macro = "INT2NUM($field_local$)";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_UINT32:
                case google::protobuf::FieldDescriptor::Type::TYPE_FIXED32:
                    conversion_macro = "UINT2NUM($field_local$)";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_INT64:
                case google::protobuf::FieldDescriptor::Type::TYPE_SFIXED64:
                    conversion_macro = "LONG2NUM($field_local$)";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_UINT64:
                case google::protobuf::FieldDescriptor::Type::TYPE_FIXED64:
                    conversion_macro = "ULONG2NUM($field_local$)";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_FLOAT:
                    conversion_macro = "DBL2NUM($field_local$)";
                    break;
                case google::protobuf::FieldDescriptor::Type::TYPE_DOUBLE:
                    conversion_macro = "DBL2NUM($field_local$)";
                    break;
                default:
                    break;
            }

            printer.Print(
                ("auto cpp_value = cpp_self->cpp_proto.$field_name$();\n"
                "return " + conversion_macro + ";\n").c_str(),
                "field_name", field->name(),
                "field_local", "cpp_value"
            );

            printer.Outdent(); printer.Outdent();
            printer.Print("}\n");
        }
        printer.Print("\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_allocators(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // The message needs an alloc function, and a free function, and a mark function, for ruby.
        // It also needs a static initialize method to use as a factory.
        printer.Print(
            "static VALUE alloc(VALUE self) {\n"
            "    auto memory = ruby_xmalloc(sizeof($class_name$));\n"
            "    // Important: It guarantees that reading have_initialized returns false so we know\n"
            "    // not to run the destructor\n"
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
            "    if (obj->have_initialized) {\n"
            "        obj->~$class_name$();\n"
            "    }\n"
            "    ruby_xfree(memory);\n"
            "}\n"
            "\n"
            "static void mark(char* memory) {\n"
            "    auto cpp_this = reinterpret_cast<$class_name$*>(memory);\n"
            "    rb_gc_mark(cpp_this->last_validation_exception);\n"
            "}\n"
            "\n",
            "class_name", class_name
        );
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_validator(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {
        // Implements validate!
        printer.Print(
            "static VALUE validate(VALUE self) {\n"
            "    $class_name$* cpp_self;\n"
            "    Data_Get_Struct(self, $class_name$, cpp_self);\n"
            "    if (!NIL_P(cpp_self->last_validation_exception)) {\n"
            "        rb_exc_raise(cpp_self->last_validation_exception);\n"
            "    }\n"
            "    return Qnil;\n"
            "}\n"
            "\n",
            "class_name", class_name
        );

    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_serializer(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {

        printer.Print(
            "static VALUE serialize_to_string(VALUE self) {\n"
            "    $class_name$* cpp_self;\n"
            "    Data_Get_Struct(self, $class_name$, cpp_self);\n"
            "\n",
            "class_name", class_name
        );
        printer.Indent(); printer.Indent();

        printer.Print(
            "// More function pointer hax to avoid GVL...\n"
            "struct serialize_args {\n"
            "    decltype(cpp_self->cpp_proto)* cpp_proto;\n"
            "    size_t pb_size;\n"
            "    char* rb_buffer_ptr;\n"
            "};\n"
            "\n"
            "serialize_args args;\n"
            "args.pb_size = cpp_self->cpp_proto.ByteSize();\n"
            "VALUE rb_str = rb_str_new(\"\", 0);\n"
            "rb_str_resize(rb_str, args.pb_size);\n"
            "args.cpp_proto = &(cpp_self->cpp_proto);\n"
            "args.rb_buffer_ptr = RSTRING_PTR(rb_str);\n"
            "\n"
            "// We want to call proto.SerializeToArray outside of the ruby GVL.\n"
            "// We can't convert a lambda that captures to a function pointer, so we\n"
            "// do stupid struct hacks.\n"
            "rb_thread_call_without_gvl(\n"
            "    [](void* _args_void) -> void* {\n"
            "        auto _args = reinterpret_cast<serialize_args*>(_args_void);\n"
            "        _args->cpp_proto->SerializeToArray(_args->rb_buffer_ptr, static_cast<int>(_args->pb_size));\n"
            "        return nullptr;\n"
            "    },\n"
            "    &args, RUBY_UBF_IO, nullptr\n"
            ");\n"
            "return rb_str;\n"
        );

        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_message_struct_parser(
        const google::protobuf::FileDescriptor* file,
        const google::protobuf::Descriptor* message_type,
        const std::string &class_name,
        google::protobuf::io::Printer &printer
    ) const {

        printer.Print(
            "static VALUE parse(VALUE self, VALUE io) {\n"
            "    $class_name$* cpp_self;\n"
            "    Data_Get_Struct(self, $class_name$, cpp_self);\n"
            "\n",
            "class_name", class_name
        );
        printer.Indent(); printer.Indent();

        printer.Print(
            "// More function pointer hax to avoid GVL...\n"
            "struct parse_args {\n"
            "    decltype(cpp_self->cpp_proto)* cpp_proto;\n"
            "    size_t pb_size;\n"
            "    char* rb_buffer_ptr;\n"
            "};\n"
            "\n"
            "parse_args args;\n"
            "args.cpp_proto = &(cpp_self->cpp_proto);\n"
            "// Convert the ruby IO to a string\n"
            "VALUE io_string = rb_funcall(io, rb_intern(\"read\"), 0);\n"
            "args.pb_size = RSTRING_LEN(io_string);\n"
            "args.rb_buffer_ptr = RSTRING_PTR(io_string);\n"
            "\n"
            "// We want to call proto.ParseFromArray outside of the ruby GVL.\n"
            "// We can't convert a lambda that captures to a function pointer, so we\n"
            "// do stupid struct hacks.\n"
            "rb_thread_call_without_gvl(\n"
            "    [](void* _args_void) -> void* {\n"
            "        auto _args = reinterpret_cast<parse_args*>(_args_void);\n"
            "        _args->cpp_proto->ParseFromArray(_args->rb_buffer_ptr, static_cast<int>(_args->pb_size));\n"
            "        return nullptr;\n"
            "    },\n"
            "    &args, RUBY_UBF_IO, nullptr\n"
            ");\n"
            "RB_GC_GUARD(io_string);\n"
            "return Qnil;\n"
        );

        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");
    }

    void RBFastProtoCodeGenerator::write_cpp_message_module_init(
        const google::protobuf::FileDescriptor* file,
        const std::vector<std::string> &class_names,
        google::protobuf::io::Printer &printer
    ) const {
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

        for (auto&& class_name : class_names) {

            printer.Print(
                "$class_name$::initialize_class();\n",
                "class_name", class_name
            );
        }

        // Close off the _Init_header_name function
        printer.Outdent(); printer.Outdent();
        printer.Print("}\n");
    }

}
